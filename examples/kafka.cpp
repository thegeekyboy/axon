#include <iostream>
#include <fstream>
#include <algorithm>
#include <variant>

#include <cmath>

#include <axon.h>
#include <axon/kafka.h>
#include <axon/redis.h>
#include <axon/util.h>
// #include <axon/database.h>
#include <axon/oracle.h>
#include <axon/scylladb.h>

#include <signal.h>

#include "entity.h"
// #include "exprtk.hpp"
#include "logica.h"

class transaction_type_code {

	typedef struct {
		uint32_t REASON_TYPE_ID;
		uint16_t TRX_TYPE_CODE;
		std::string TRX_TYPE_DESC;
	} trx_type_map;

	std::vector<trx_type_map> _data;

	bool _load_file(std::string filename) {
		std::ifstream file(filename);
		std::string line;

		if (!file) return false;

		while (std::getline(file, line))
		{
			std::stringstream ss(line);
			std::string field;
			trx_type_map t;

			// REASON_TYPE_ID
			std::getline(ss, field, ',');
			t.REASON_TYPE_ID = std::stoi(field);

			// TRX_TYPE_CODE
			std::getline(ss, field, ',');
			t.TRX_TYPE_CODE = std::stoi(field);

			// TRX_TYPE_DESC
			std::getline(ss, t.TRX_TYPE_DESC, ',');

			_data.push_back(t);
		}
		return true;
	};
	
	public:

		transaction_type_code(std::string filename) {
			if (_load_file(filename)) std::sort(_data.begin(), _data.end(), [](const trx_type_map& lhs, const trx_type_map& rhs) { return lhs.TRX_TYPE_DESC < rhs.TRX_TYPE_DESC; });
		}

		trx_type_map *find(uint32_t id) {
			auto it = std::find_if(_data.begin(), _data.end(), [id](const trx_type_map& u) {
				return u.REASON_TYPE_ID == id;
			});

			if (it != _data.end()) return std::addressof(*it);

			return nullptr;
		}

		std::string name(uint32_t id) {
			if (auto elm = find(id)) return elm->TRX_TYPE_DESC;
			return std::string();
		}

		uint16_t code(uint32_t id) {
			if (auto elm = find(id)) return elm->TRX_TYPE_CODE;
			return 0;
		}
		
		trx_type_map &get(size_t index) {
			if (index < _data.size()) return _data[index];
			throw std::invalid_argument("index out of range");
		}

		size_t size() { return _data.size(); }
		void print() {
			for (const auto& elm : _data) {
				std::cout<<elm.REASON_TYPE_ID<<","<<elm.TRX_TYPE_CODE<<","<<elm.TRX_TYPE_DESC<<std::endl;
			}
		}
};

std::shared_ptr<transaction_type_code> typemaps;

Stats query(axon::cache::redis &redis, uint64_t identity_id, uint8_t wsize = 24)
{
	static int TYPES[5] = { 1000, 1001, 1002, 1003, 1009 };
	std::time_t hour = std::time(nullptr);//(std::time(nullptr) % 3600);
	std::string key;

	Stats st;

	// std::cout<<std::time(nullptr)<<std::endl;

	// for (size_t i = 0; i < typemaps->size(); i++)
	for ( auto &tc: TYPES)
	{
		// uint16_t tc = typemaps->get(i).TRX_TYPE_CODE;
		std::time_t hts = hour;

		redis.pipeline_begin();
		for (int i = 0; i <= wsize; i++)
		{
			std::tm tm_buf {};

			localtime_r(&hts, &tm_buf);
			char hour_str[16];
			std::strftime(hour_str, sizeof(hour_str), "%Y%m%d%H", &tm_buf);

			key = "entity:" + std::to_string(identity_id) + ":" + std::to_string(tc) + ":" + hour_str;
			redis.pipeline_hgetall(key);
			hts -= 3600;
		}

		std::vector<axon::cache::reply> response = redis.pipeline_run();

		for (auto &r : response)
		{
			if (r.ok() && !r.is_null() && r->type == REDIS_REPLY_ARRAY && r->elements > 0)
			{
				printf("total fields: %zu\n", r->elements / 2);

				for (size_t i = 0; i + 1 < r->elements; i += 2)
				{
					std::string_view field(r->element[i]->str,   r->element[i]->len);
					std::string_view value(r->element[i+1]->str, r->element[i+1]->len);

					printf("  field=%-12.*s  value=%.*s\n",
						(int)field.size(), field.data(),
						(int)value.size(), value.data());

					switch (tc)
					{
						case 1000:
							if (field == "count") st.ci+=axon::util::str_to_num<long>(value);
							if (field == "sum") st.ci_sum+=axon::util::str_to_num<long>(value);
							break;
						case 1001:
							if (field == "count") st.co+=axon::util::str_to_num<long>(value);
							if (field == "sum") st.co_sum+=axon::util::str_to_num<long>(value);
							break;
						case 1002:
							if (field == "count") st.p2p+=axon::util::str_to_num<long>(value);
							if (field == "sum") st.p2p_sum+=axon::util::str_to_num<long>(value);
							break;
						case 1003:
							if (field == "count") st.pay+=axon::util::str_to_num<long>(value);
							if (field == "sum") st.pay_sum+=axon::util::str_to_num<long>(value);
							break;
						case 1009:
							if (field == "count") st.bill+=axon::util::str_to_num<long>(value);
							if (field == "sum") st.bill_sum+=axon::util::str_to_num<long>(value);
							break;
					}
				}
			}
		}
	}

	return st;
}

/// globals
static bool running = false;
static unsigned long count = 0;
static long delta = 0;

std::shared_ptr<cache> memcache;
///

static void stop (int sig)
{
	running = false;
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);
}

static void reload (int sig)
{
	running = false;
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);
}

void counter([[maybe_unused]] axon::stream::kafka *hook)
{
	long tick = axon::timer::epoch();

	running = true;
	while (running)
	{
		std::this_thread::sleep_for(std::chrono::seconds(10));

		long tt = axon::timer::epoch();
		float tps = count/(tt-tick);
		tick = tt;
		long apt = (count)?delta/count:0;

		std::cout<<"count: "<<count<<", "<<tps<<" rps, id count: "<<memcache->size()<<", memory: "<<memcache->memory()<<", apt: "<<apt<<std::endl;

		count = 0;
		delta = 0;
	}

	// hook->stop();
}

void parse(std::shared_ptr<axon::stream::recordset> rec, axon::cache::redis &redis)
{
	axon::timer ctm(__PRETTY_FUNCTION__);
	std::string table, op_type;

	// std::shared_ptr<axon::database::tableinfo> inf = db->getinfo("cps_transaction_normalized");

	// std::string ORDERID, TRANS_STATUS, DEBIT_PARTY_TYPE, DEBIT_PARTY_MNEMONIC, CREDIT_PARTY_TYPE, CREDIT_PARTY_MNEMONIC, REQUEST_CURRENCY, ACCOUNT_UNIT_TYPE, CURRENCY, IS_REVERSED, IS_PARTIAL_REVERSED, IS_REVERSING, REMARK, BANK_ACCOUNT_NUMBER, BANK_ACCOUNT_NAME, CONSUMED_BUNDLE;
	// long long TRANS_INITATE_TIME = 0, TRANS_END_TIME = 0, EXPIRED_TIME = 0, LAST_UPDATED_TIME = 0, REASON_TYPE = 0;
	// long long REQUEST_AMOUNT = 0, ORG_AMOUNT = 0, ACTUAL_AMOUNT = 0, FEE = 0, COMMISSION = 0, TAX = 0, DISCOUNT_AMOUNT = 0, REDEEMED_POINT_AMOUNT = 0, EXCHANGE_RATE = 0;
	// std::string CHANNEL, ORIGCONVERSATIONID, LINKEDTYPE, LINKEDORDERID, REQUESTER_TYPE, REQUESTER_IDENTIFIER_TYPE, REQUESTER_IDENTIFIER_VALUE, REQUESTER_MNEMORIC, INITIATOR_TYPE, INITIATOR_IDENTIFIER_TYPE, INITIATOR_IDENTIFIER_VALUE, INITIATOR_MNEMONIC, PRIMARY_PARTY_TYPE, THIRDPARTYID, THIRDPARTYIP, ACCESSPOINTIP, THIRDPARTYREQTIME, ERRORCODE, ERRORMESSAGE;
	
	uint8_t IS_REVERSED;
	uint64_t DEBIT_PARTY_ID = 0, CREDIT_PARTY_ID = 0;
	long ACTUAL_AMOUNT = 0, REASON_TYPE = 0, ORG_AMOUNT = 0, FEE = 0, COMMISSION = 0;
	long long TRANS_INITATE_TIME = 0, TRANS_END_TIME = 0;
	std::string ORDERID, TRANS_STATUS, DEBIT_PARTY_MNEMONIC, CREDIT_PARTY_MNEMONIC;

	std::string sbuf;
	char buffer[16] = { 0 };
	size_t size;

	std::string REFERENCE_KEY, REFERENCE_VALUE;

	if (rec->name() == "CPS_TRANS_RECORD")
	{
		// rec->get("table", table);
		// rec->get("op_type", op_type);
		rec->get("ORDERID", ORDERID);
		rec->get("TRANS_STATUS", TRANS_STATUS);
		// rec->get("DEBIT_PARTY_TYPE", DEBIT_PARTY_TYPE);
		rec->get("DEBIT_PARTY_MNEMONIC", DEBIT_PARTY_MNEMONIC); std::replace(DEBIT_PARTY_MNEMONIC.begin(), DEBIT_PARTY_MNEMONIC.end(), '\'', '*');
		// rec->get("CREDIT_PARTY_TYPE", CREDIT_PARTY_TYPE);
		rec->get("CREDIT_PARTY_MNEMONIC", CREDIT_PARTY_MNEMONIC);
		// rec->get("REQUEST_CURRENCY", REQUEST_CURRENCY);
		// rec->get("ACCOUNT_UNIT_TYPE", ACCOUNT_UNIT_TYPE);
		// rec->get("CURRENCY", CURRENCY);
		// rec->get("REMARK", REMARK);
		rec->get("IS_REVERSED", sbuf); IS_REVERSED = axon::util::str_to_num<uint8_t>(sbuf);
		// rec->get("IS_PARTIAL_REVERSED", IS_PARTIAL_REVERSED);
		// rec->get("IS_REVERSING", IS_REVERSING);
		// rec->get("BANK_ACCOUNT_NUMBER", BANK_ACCOUNT_NUMBER);
		// rec->get("BANK_ACCOUNT_NAME", BANK_ACCOUNT_NAME);
		// rec->get("CONSUMED_BUNDLE", CONSUMED_BUNDLE);

		rec->get("TRANS_INITATE_TIME", TRANS_INITATE_TIME);
		rec->get("TRANS_END_TIME", TRANS_END_TIME);
		// rec->get("EXPIRED_TIME", EXPIRED_TIME);
		// rec->get("LAST_UPDATED_TIME", LAST_UPDATED_TIME);
		// rec->get("REQUEST_AMOUNT", REQUEST_AMOUNT);
		rec->get("ORG_AMOUNT", ORG_AMOUNT);
		rec->get("ACTUAL_AMOUNT", ACTUAL_AMOUNT);
		rec->get("FEE", FEE);
		rec->get("COMMISSION", COMMISSION);
		// rec->get("TAX", TAX);
		// rec->get("DISCOUNT_AMOUNT", DISCOUNT_AMOUNT);
		// rec->get("REDEEMED_POINT_AMOUNT", REDEEMED_POINT_AMOUNT);

		if ((size = rec->get("DEBIT_PARTY_ID", buffer, 8)) > 0) DEBIT_PARTY_ID = axon::util::bytes_to_ull(buffer, size);
		if ((size = rec->get("CREDIT_PARTY_ID", buffer, 8)) > 0) CREDIT_PARTY_ID = axon::util::bytes_to_ull(buffer, size);
		if ((size = rec->get("REASON_TYPE", buffer, 8)) > 0) REASON_TYPE = axon::util::bytes_to_ull(buffer, size);
		// if ((size = rec->get("CREDIT_PARTY_ID", buffer, 8)) > 0) CREDIT_PARTY_ID = std::to_string(axon::util::bytes_to_ull(buffer, size));
		// if ((size = rec->get("EXCHANGE_RATE", buffer, 8)) > 0) EXCHANGE_RATE = axon::util::bytes_to_ull(buffer, size);
	
		// std::cout<<axon::timer::fulldate(TRANS_INITATE_TIME)<<">>"<<ORDERID<<" ("<<op_type<<") - "<<REASON_TYPE<<std::endl;
		// std::cout<<ORDERID<<" ("<<op_type<<") - "<<REASON_TYPE<<" => "<<DEBIT_PARTY_ID<<"::"<<CREDIT_PARTY_ID<<std::endl;
		// std::cout<<ORDERID<<" ("<<op_type<<") - "<<REASON_TYPE<<" => "<<typemaps->name(REASON_TYPE)<<std::endl;

		// std::cout<<TRANS_STATUS<<"<::>"<<IS_REVERSED<<"<::>"<<DEBIT_PARTY_ID<<"<::>"<<CREDIT_PARTY_ID<<"<::>"<<ACTUAL_AMOUNT<<"<::>"<<REASON_TYPE<<"<::>"<<ORG_AMOUNT<<"<::>"<<FEE<<"<::>"<<COMMISSION<<"<::>"<<TRANS_INITATE_TIME<<"<::>"<<TRANS_END_TIME<<"<::>"<<ORDERID<<"<::>"<<DEBIT_PARTY_MNEMONIC<<"<::>"<<CREDIT_PARTY_MNEMONIC<<std::endl;

		uint16_t typecode = typemaps->code(REASON_TYPE);

		/*
		try {
			entity *ptr = nullptr;

			if (!(ptr = memcache->find(DEBIT_PARTY_ID))) {
				ptr = memcache->push(DEBIT_PARTY_ID);
			}

			
		} catch (const std::invalid_argument& e) {
		}
		*/

		time_t epochSec = TRANS_INITATE_TIME / 1000;
    	struct tm *t = localtime(&epochSec);
		std::string key;
	    char buf[11];
		
		strftime(buf, sizeof(buf), "%Y%m%d%H", t);

		// if (typecode > 1)
		// {
		// 	key = "entity:" + std::to_string(DEBIT_PARTY_ID) + ":" + std::to_string(typecode) + ":" + buf;

		// 	redis.pipeline_begin();
		// 	redis.pipeline_hincrby(key, "count", 1);
		// 	redis.pipeline_hincrby(key, "sum", ACTUAL_AMOUNT);
		// 	redis.pipeline_expire(key, 262800);   // 73hr TTL — auto-expiry
		// 	redis.pipeline_commit();

		// 	Stats st = query(redis, DEBIT_PARTY_ID, 6);

		// 	std::cout<<DEBIT_PARTY_ID<<"+>CI:"<<st.ci<<
		// }

		// {
		// 	axon::timer ctm(__PRETTY_FUNCTION__);

		// 	exprtk::symbol_table<double> symbol_table;
		// 	double aa = ACTUAL_AMOUNT/100, ir = IS_REVERSED;
			
		// 	symbol_table.add_variable("TRX_TYPE_CODE", typecode);
		// 	symbol_table.add_variable("ACTUAL_AMOUNT", aa);
		// 	symbol_table.add_variable("IS_REVERSED", ir);

		// 	exprtk::expression<double> expression;
		// 	expression.register_symbol_table(symbol_table);

		// 	exprtk::parser<double> parser;
		// 	parser.compile("ACTUAL_AMOUNT >= 2190.94 and TRX_TYPE_CODE = 1000", expression);

		// 	double result = expression.value();
		// 	delta += ctm.now();
		// }

		{
			axon::logica::Transaction txn;

			std::strncpy(txn.orderid, ORDERID.data(), sizeof(txn.orderid) - 1);
			txn.trans_status = 1;
			txn.trans_initate_time = TRANS_INITATE_TIME;
			txn.trans_end_time = TRANS_END_TIME;
			txn.debit_party_id = DEBIT_PARTY_ID;
			std::strncpy(txn.debit_party_mnemonic,  DEBIT_PARTY_MNEMONIC.data(), sizeof(txn.debit_party_mnemonic) - 1);
			txn.credit_party_id = CREDIT_PARTY_ID;
			std::strncpy(txn.credit_party_mnemonic, CREDIT_PARTY_MNEMONIC.data(), sizeof(txn.credit_party_mnemonic) - 1);
			txn.reason_type = REASON_TYPE;
			txn.type_code = typecode;
			txn.org_amount = ORG_AMOUNT/100;
			txn.actual_amount = ACTUAL_AMOUNT/100;
			txn.fee = FEE/100;
			txn.commission = COMMISSION/100;
			txn.is_reversed = IS_REVERSED;

			char expression[256] = "actual_amount >= 2190.94 and type_code == 1002";

			try {
				auto ast = axon::logica::compile(expression);
				bool answer = axon::logica::Evaluator::eval(*ast, txn);

				// if (answer) std::cout<<TRANS_STATUS<<"<::>"<<IS_REVERSED<<"<::>"<<DEBIT_PARTY_ID<<"<::>"<<CREDIT_PARTY_ID<<"<::>"<<ACTUAL_AMOUNT<<"<::>"<<REASON_TYPE<<"<::>"<<ORG_AMOUNT<<"<::>"<<FEE<<"<::>"<<COMMISSION<<"<::>"<<TRANS_INITATE_TIME<<"<::>"<<TRANS_END_TIME<<"<::>"<<ORDERID<<"<::>"<<DEBIT_PARTY_MNEMONIC<<"<::>"<<CREDIT_PARTY_MNEMONIC<<std::endl;
			} catch (const axon::logica::ParseError& e) {
				std::cout << "PARSE ERROR: " << e.what() << "\n";
			} catch (const axon::logica::EvalError& e) {
				std::cout << "EVAL  ERROR: " << e.what() << "\n";
			}
		}

		// if (result) std::cout<<REASON_TYPE<<"<>"<<DEBIT_PARTY_ID<<"::"<<typecode<<"::"<<aa<<std::endl;
	}

	delta += ctm.now();
	count++;
}

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], [[maybe_unused]]char* env[])
{
	const char *envp;
	std::string hostname, username, password, schema_registry, domain, ora_sid, krb5_keytab, krb5_cachepath, bootstrap, kafka_consumer_group, scylla_keyspace, proxy;

	if ((envp = std::getenv("http_proxy")) != nullptr) proxy = envp;
	if ((envp = std::getenv("AXON_DOMAIN")) != nullptr) domain = envp;
	if ((envp = std::getenv("AXON_ORA_SID")) != nullptr) ora_sid = envp;
	if ((envp = std::getenv("AXON_USERNAME")) != nullptr) username = envp;
	if ((envp = std::getenv("AXON_PASSWORD")) != nullptr) password = envp;
	if ((envp = std::getenv("AXON_HOSTNAME")) != nullptr) hostname = envp;
	if ((envp = std::getenv("AXON_BOOTSTRAP")) != nullptr) bootstrap = envp;
	if ((envp = std::getenv("AXON_KRB5_KEYTAB")) != nullptr) krb5_keytab = envp;
	if ((envp = std::getenv("AXON_KRB5_CACHEPATH")) != nullptr) krb5_cachepath = envp;
	if ((envp = std::getenv("AXON_SCHEMA_REGISTRY")) != nullptr) schema_registry = envp;
	if ((envp = std::getenv("AXON_SCYLLA_KEYSPACE")) != nullptr) scylla_keyspace = envp;
	if ((envp = std::getenv("AXON_KAFKA_CONSUMER_GROUP")) != nullptr) kafka_consumer_group = envp;

	// if (argc <= 2) return 0;

	typemaps = std::make_shared<transaction_type_code>("/home/amirul.islam/axon/examples/DWD_TRX_TYPE_MAP.csv");
	memcache = std::make_shared<cache>();

	axon::stream::kafka source(bootstrap, schema_registry, "hyperion");

	axon::cache::redis redis;

	// axon::cache::redis redis("10.82.30.32", 6379);
	// redis.login("sentinel", "??");

	std::thread th(counter, &source);

	// source.add("CPS_ORDERHIS");
	// source.add("CPS_ORDER_REFDATA");
	source.add("CPS_TRANS_RECORD");
	// source.add("UAT-USER-FP-STAT-STREAM");
	source.subscribe();

	signal(SIGINT, stop);
	signal(SIGHUP, reload);

	// axon::database::oracle ora;
	// ora.connect(ora_sid, username, password);
	// axon::database::interface &db = ora;

	// axon::database::scylladb db;
	// db[AXON_DATABASE_KEYSPACE] = scylla_keyspace;
	// db.connect(hostname, username, password);

	std::shared_ptr<axon::stream::recordset> rc;
	
	// parse by polling
	/*
	while (running && (rc = std::move(source.next())))
	{
		if (rc->is_empty())
			continue;
		// parse(rc, db);
		parse(rc);
	}
	*/

	// parse by callback
	source.start([&](std::unique_ptr<axon::stream::recordset> rec) {
		rc = std::move(rec);
		parse(rc, redis);
		// rc->print();
	});

	th.join();

	// source.unsubscribe();
	source.stop();

	// memcache->print();

	return 0;
}