#ifndef AXON_TELECOM_H_
#define AXON_TELECOM_H_

namespace axon
{
	namespace telecom
	{
		typedef struct
		{
			std::string msisdn;
			std::string imsi;
			std::string imei;
			std::string vlr;
			struct std::tm lastactive;
			int lac;
			int cell;
		} subscriber;

		typedef struct {

			int lastlac;
			int lastcell;
			struct std::tm lastactive;
			short int lastaction;
			unsigned short int cnt_moc;
			unsigned short int cnt_mtc;
			unsigned short int cnt_smo;
			unsigned short int cnt_smt;
		} substat;
	}
}

#endif