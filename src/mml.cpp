#include <axon.h>
#include <axon/socket.h>
#include <axon/telecom.h>
#include <axon/mml.h>

namespace axon
{
	namespace telecom {

		namespace core {

			mml::mml()
			{
				_success = boost::regex("^RETCODE = ([0-9]{1,9})\\s+(.*)");
				_break = boost::regex("^---\\s+END");
				_error = boost::regex("^ERROR$");
			}

			mml::~mml()
			{
				_sock.stop();

				usleep(100000);

				if (_th.joinable())
					_th.join();
			}

			bool mml::connect(std::string hostname, unsigned short int port)
			{
				_hostname = hostname;
				_port = port;

				_sock.init();
				_sock.open(_hostname, _port);

				try {

					_th = std::thread(&axon::transport::tcpip::socks::readline, &_sock);

				} catch(std::exception &e) {

					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Generic: Encountered exception while creating thread!");
				}

				return true;
			}

			bool mml::disconnect()
			{

				return true;
			}

			bool mml::conditions(std::string success, std::string error, std::string breaks)
			{
				try {

					_success = boost::regex(success);
					_error = boost::regex(error);
					_break = boost::regex(breaks);

				} catch (boost::regex_error &e) {

					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Regex: Error compiling condition expression");
				}

				return true;
			}

			/*
			 * mml::exec - Execute MML Instruction
			 *
			 * cmd = Command to be run on the network element
			 * vec = A string vector which is passed by reference by the callee and will be populated with responses from the NE
			 * timeout = timeout in second for which exec will wait and return -1 if timed out
			 * cb = Pointer to a function which will be called each time the NE send a line of response.
			 *
			 * callback function signature = int function(std::string response, short int &retval);
			 * 
			 * Example lambda callback
			 *
			 * short int i = exec("LGI: HLRSN=1, OPNAME=\"" + _username + "\", PWD=\"" + _password + "\";", [](std::string resp, short int& retval) {
			 * 	   ...
			 * }
			 *
			 * Return values from callback (return true to break; false to continue)
			 *
			 * retval =
			 *	-1 = timeout (not to be set)
			 *	-2 = matched error string
			 *	0 = generic success
			 *	>0 = success code
			 */

			short int mml::exec(std::string cmd, std::vector<std::string> *vec, unsigned short int timeout, const std::function <int (std::string, short int&)>& cb)
			{
				short int retval = -1;
				bool running = true, looping = true, finished = false;
				std::thread timeth;

				_sock.writeline(cmd);

				if (timeout > 0)
				{
					timeth = std::thread([&timeout, &running, &looping, &finished, &retval] () {

						while (looping)
						{
							long xt = ((timeout * 1000000) / 10000);
							looping = false;

							for (; xt > 0 && !finished; xt--)
								usleep(10000);
						}

						if (!finished) retval = -3;
						running = false;
					});
				}

				while ((_sock.alive() || _sock.linewaiting()) && running)
				{
					if (_sock.linewaiting())
					{
						looping = true;

						std::string resp = _sock.line();

						if (resp.size() > 1)
						{
							std::cout<<"Debug>"<<resp<<"<"<<std::endl;
							boost::smatch what;

							if (vec != NULL) vec->push_back(resp);

							if (!cb)
							{
								if (boost::regex_match(resp, what, _success))
								{
									if (what.size() >= 2)
										retval = stoi(what[1]);
									else
										retval = 0;
								}

								if (boost::regex_match(resp, _error))
								{
									finished = true;
									retval = -2;
									break;
								}

								if (boost::regex_match(resp, _break))
								{
									finished = true;
									break;
								}
							} else {

								if (cb(resp, retval))
								{
									finished = true;
									break;
								}
							}
						}
					}
					usleep(5000);
				}

				if (timeout > 0)
					timeth.join();

				return retval;
			}

			//
			// Class : huamsc
			//

			bool huamsc::login(std::string username, std::string password)
			{
				std::vector<std::string> response;

				_username = username;
				_password = password;

				if (exec("LGI:OP=\"" + _username + "\", PWD=\"" + _password + "\";", &response, 5) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Login error!");

				return true;
			}

			bool huamsc::logout()
			{
				if (_msc.size() > 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Still registered with " + _msc + ", unregister first.");

				if (exec("LGO:OP=\"" + _username + "\";") != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Logout error");

				return true;
			}

			bool huamsc::reg(std::string neid)
			{

				if (_msc.size() > 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Already registered with " + _msc);

				if (exec("REG NE:NAME=\"" + neid + "\";") != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Error connecting to Network Element");

				_msc = neid;

				return true;
			}

			bool huamsc::unreg()
			{
				if (_msc.size() <= 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Not registered with any MSC.");

				if (exec("UNREG NE:NAME=\"" + _msc + "\";") != 0)
					return false;

				_msc.clear();

				return true;
			}

			short int huamsc::addclrdsg(std::string msisdn, unsigned short int dsp)
			{
				std::vector<std::string> response;

				if (_msc.size() <= 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Not registered with any MSC.");

				int retval = exec("ADD CLRDSG: DSP=" + std::to_string(dsp) + ", CLI=K'" + msisdn + ", DAI=ALL, FUNC=NIN;", &response);

				if (retval == 0)
					return 0;
				else if (retval == 5952 || retval == 245952)
					return 1;
				else
					return -1;
			}

			short int huamsc::delclrdsg(std::string msisdn, unsigned short int dsp)
			{
				std::vector<std::string> response;

				if (_msc.size() <= 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Not registered with any MSC.");

				int retval = exec("RMV CLRDSG: DSP=" + std::to_string(dsp) + ", CLI=K'" + msisdn + ", DAI=ALL;", &response);

				if (retval == 0)
					return 0;
				else if (retval == 5952 || retval == 245952)
					return 1;
				else
					return -1;
			}

			bool huamsc::query(telecom::subscriber &subs)
			{
				std::vector<std::string> response;

				if (_msc.size() <= 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Not registered with any MSC.");

				if (subs.msisdn.size() == 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "HLR: Invalid MSISDN");

				if (exec("DSP USRINF: UNT=MSISDN, D=K'" + subs.msisdn + ";", &response) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "MSC: Error connecting to Network Element");

				for (unsigned int i = 0; i < response.size(); i++)
				{
					boost::smatch what;

					if (boost::regex_match(response[i], what, boost::regex("^\\s+IMSI\\s+=\\s+([0-9]+)")))
					{
						subs.imsi = what[1];
					}

					if (boost::regex_match(response[i], what, boost::regex("^\\s+IMEI\\s+=\\s+([0-9]+)")))
					{
						subs.imei = what[1];
					}

					if (boost::regex_match(response[i], what, boost::regex("^\\s+Cell\\sIdentity\\s+=\\s+41220([a-zA-Z0-9]{4})([a-zA-Z0-9]{4})")))
					{
						std::string tmp = what[1];
						subs.lac = strtol(tmp.c_str(), NULL, 16);

						tmp = what[2];
						subs.cell = strtol(tmp.c_str(), NULL, 16);
					}

					if (boost::regex_match(response[i], what, boost::regex(".*action\\s+=\\s+([0-9]{4}-[0-9]{2}-[0-9]{2}\\s[0-9]{2}:[0-9]{2}:[0-9]{2}).*")))
					{
						std::string datex = what[1];
						strptime(datex.c_str(), "%Y-%m-%d %H:%M:%S", &(subs.lastactive));
					}
				}

				return true;
			}

			//
			// Class : huahlr
			//

			bool huahlr::login(std::string username, std::string password)
			{
				std::vector<std::string> response;

				_username = username;
				_password = password;

				if (exec("LGI: HLRSN=1, OPNAME=\"" + _username + "\", PWD=\"" + _password + "\";") != 1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "HLR: Login error");

				return true;
			}

			bool huahlr::logout()
			{
				if (exec("LGO;") != 1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "HLR: Logout error");

				return true;
			}

			bool huahlr::remove(std::string msisdn)
			{
				if (exec("RMV SUB: ISDN=\"" + msisdn + "\", RMVKI=TRUE;") != 1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "HLR: Error removing subscriber");

				return false;
			}

			bool huahlr::vlrnum(telecom::subscriber &subs)
			{
				std::vector<std::string> response;

				if (subs.msisdn.size() == 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "HLR: Invalid MSISDN");

				if (exec("LST VLRNUM: ISDN=\"" + subs.msisdn + "\";", &response) != 1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "HLR: Error performing query on subscriber");

				for (unsigned int i = 0; i < response.size(); i++)
				{
					boost::smatch what;

					if (boost::regex_match(response[i], what, boost::regex("^\\s+IMSI\\s+=\\s+([0-9]+)")))
					{
						subs.imsi = what[1];
					}

					if (boost::regex_match(response[i], what, boost::regex("^\\s+VLRNUM\\s+=\\s+([0-9]+)")))
					{
						subs.vlr = what[1];
					}
				}

				return true;
			}
		}
	}
}