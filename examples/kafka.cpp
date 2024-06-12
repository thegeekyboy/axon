#include <iostream>
#include <algorithm>
#include <variant>

#include <cmath>

#include <axon.h>
#include <axon/kafka.h>
#include <axon/util.h>
// #include <axon/database.h>
#include <axon/scylladb.h>

#include <signal.h>

extern "C" {
	#include <avro.h>
	#include <libserdes/serdes.h>
	#include <libserdes/serdes-avro.h>
}

#include <cassandra.h>
#include <dse.h>

typedef std::variant<std::nullptr_t, std::string, double, int64_t, int, bool> avt;

static bool running = false;
static unsigned long count = 0;

static void stop (int sig)
{
	running = false;
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);
}

void counter(axon::stream::kafka *hook)
{
	long tick = axon::timer::epoch();

	running = true;
	while (running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

		long tt = axon::timer::epoch();
		float tps = count/(tt-tick);
		tick = tt;

		std::cout<<"count: "<<count<<", "<<tps<<" rps"<<std::endl;

		count = 0;
	}

	hook->stop();
}

void parse(std::shared_ptr<axon::stream::recordset> rec, axon::database::scylladb *db)
{
	std::string table, op_type;

	// std::shared_ptr<axon::database::tableinfo> inf = db->getinfo("cps_transaction_normalized");

	rec->get("table", table);
	rec->get("op_type", op_type);

	std::string ORDERID, TRANS_STATUS, DEBIT_PARTY_TYPE, DEBIT_PARTY_MNEMONIC, CREDIT_PARTY_TYPE, CREDIT_PARTY_MNEMONIC, REQUEST_CURRENCY, ACCOUNT_UNIT_TYPE, CURRENCY, IS_REVERSED, IS_PARTIAL_REVERSED, IS_REVERSING, REMARK, BANK_ACCOUNT_NUMBER, BANK_ACCOUNT_NAME, CONSUMED_BUNDLE;
	long long TRANS_INITATE_TIME = 0, TRANS_END_TIME = 0, EXPIRED_TIME = 0, LAST_UPDATED_TIME = 0;
	long long REQUEST_AMOUNT = 0, ORG_AMOUNT = 0, ACTUAL_AMOUNT = 0, FEE = 0, COMMISSION = 0, TAX = 0, DISCOUNT_AMOUNT = 0, REDEEMED_POINT_AMOUNT = 0, EXCHANGE_RATE = 0;
	std::string DEBIT_PARTY_ID, CREDIT_PARTY_ID, REASON_TYPE;
	char buffer[16] = { 0 };
	size_t size;
	
	std::string CHANNEL, ORIGCONVERSATIONID, LINKEDTYPE, LINKEDORDERID, REQUESTER_TYPE, REQUESTER_IDENTIFIER_TYPE, REQUESTER_IDENTIFIER_VALUE, REQUESTER_MNEMORIC, INITIATOR_TYPE, INITIATOR_IDENTIFIER_TYPE, INITIATOR_IDENTIFIER_VALUE, INITIATOR_MNEMONIC, PRIMARY_PARTY_TYPE, THIRDPARTYID, THIRDPARTYIP, ACCESSPOINTIP, THIRDPARTYREQTIME, ERRORCODE, ERRORMESSAGE;

	std::string REFERENCE_KEY, REFERENCE_VALUE;

	if (table == "CPS_TRANS_RECORD-VALUE.CPS_TRANS_RECORD")
	{
		rec->get("ORDERID", ORDERID);
		rec->get("TRANS_STATUS", TRANS_STATUS);
		rec->get("DEBIT_PARTY_TYPE", DEBIT_PARTY_TYPE);
		rec->get("DEBIT_PARTY_MNEMONIC", DEBIT_PARTY_MNEMONIC); std::replace(DEBIT_PARTY_MNEMONIC.begin(), DEBIT_PARTY_MNEMONIC.end(), '\'', '*');
		rec->get("CREDIT_PARTY_TYPE", CREDIT_PARTY_TYPE);
		rec->get("CREDIT_PARTY_MNEMONIC", CREDIT_PARTY_MNEMONIC);
		rec->get("REQUEST_CURRENCY", REQUEST_CURRENCY);
		rec->get("ACCOUNT_UNIT_TYPE", ACCOUNT_UNIT_TYPE);
		rec->get("CURRENCY", CURRENCY);
		rec->get("REMARK", REMARK);
		rec->get("IS_REVERSED", IS_REVERSED);
		rec->get("IS_PARTIAL_REVERSED", IS_PARTIAL_REVERSED);
		rec->get("IS_REVERSING", IS_REVERSING);
		rec->get("BANK_ACCOUNT_NUMBER", BANK_ACCOUNT_NUMBER);
		rec->get("BANK_ACCOUNT_NAME", BANK_ACCOUNT_NAME);
		rec->get("CONSUMED_BUNDLE", CONSUMED_BUNDLE);

		rec->get("TRANS_INITATE_TIME", TRANS_INITATE_TIME);
		rec->get("TRANS_END_TIME", TRANS_END_TIME);
		rec->get("EXPIRED_TIME", EXPIRED_TIME);
		rec->get("LAST_UPDATED_TIME", LAST_UPDATED_TIME);
		rec->get("REQUEST_AMOUNT", REQUEST_AMOUNT);
		rec->get("ORG_AMOUNT", ORG_AMOUNT);
		rec->get("ACTUAL_AMOUNT", ACTUAL_AMOUNT);
		rec->get("FEE", FEE);
		rec->get("COMMISSION", COMMISSION);
		rec->get("TAX", TAX);
		rec->get("DISCOUNT_AMOUNT", DISCOUNT_AMOUNT);
		rec->get("REDEEMED_POINT_AMOUNT", REDEEMED_POINT_AMOUNT);

		if ((size = rec->get("DEBIT_PARTY_ID", buffer, 8)) > 0) DEBIT_PARTY_ID = std::to_string(axon::util::bytestoull(buffer, size));
		if ((size = rec->get("CREDIT_PARTY_ID", buffer, 8)) > 0) CREDIT_PARTY_ID = std::to_string(axon::util::bytestoull(buffer, size));
		if ((size = rec->get("REASON_TYPE", buffer, 8)) > 0) REASON_TYPE = std::to_string(axon::util::bytestoull(buffer, size));
		if ((size = rec->get("EXCHANGE_RATE", buffer, 8)) > 0) EXCHANGE_RATE = axon::util::bytestoull(buffer, size);

		const char stmt[] = "UPDATE CPS_TRANSACTION_NORMALIZED SET TRANS_STATUS = :aa, DEBIT_PARTY_TYPE = :ab, DEBIT_PARTY_MNEMONIC = :ac, CREDIT_PARTY_TYPE = :ad, CREDIT_PARTY_MNEMONIC = :ae, REQUEST_CURRENCY = :af, ACCOUNT_UNIT_TYPE = :ag, CURRENCY = :ah, REMARK = :ai, IS_REVERSED = :aj, IS_PARTIAL_REVERSED = :ak, IS_REVERSING = :al, BANK_ACCOUNT_NUMBER = :am, BANK_ACCOUNT_NAME = :an, CONSUMED_BUNDLE = :ao, TRANS_INITATE_TIME = :ap, TRANS_END_TIME = :aq, EXPIRED_TIME = :ar, LAST_UPDATED_TIME = :as, REQUEST_AMOUNT = :at, ORG_AMOUNT = :au, ACTUAL_AMOUNT = :av, FEE = :aw, COMMISSION = :ax, TAX = :ay, DISCOUNT_AMOUNT = :az, REDEEMED_POINT_AMOUNT = :ba, DEBIT_PARTY_ID = :bb, CREDIT_PARTY_ID = :bc, REASON_TYPE = :bd, EXCHANGE_RATE = :be WHERE ORDERID = :bf";

		(*db)<<TRANS_STATUS<<DEBIT_PARTY_TYPE<<DEBIT_PARTY_MNEMONIC<<CREDIT_PARTY_TYPE<<CREDIT_PARTY_MNEMONIC<<REQUEST_CURRENCY<<ACCOUNT_UNIT_TYPE<<CURRENCY<<REMARK<<IS_REVERSED<<IS_PARTIAL_REVERSED<<IS_REVERSING<<BANK_ACCOUNT_NUMBER<<BANK_ACCOUNT_NAME<<CONSUMED_BUNDLE<<TRANS_INITATE_TIME<<TRANS_END_TIME<<EXPIRED_TIME<<LAST_UPDATED_TIME<<REQUEST_AMOUNT<<ORG_AMOUNT<<ACTUAL_AMOUNT<<FEE<<COMMISSION<<TAX<<DISCOUNT_AMOUNT<<REDEEMED_POINT_AMOUNT<<DEBIT_PARTY_ID<<CREDIT_PARTY_ID<<REASON_TYPE<<EXCHANGE_RATE<<ORDERID;
		db->execute(stmt);
	}
	else if (table == "CPS_ORDERHIS-VALUE.CPS_ORDERHIS")
	{
		size_t size;
		char buffer[16] = { 0 };
		long ENDTIME = 0, ACCESSPOINTREQTIME = 0;
		long long REQUESTER_ID = 0, INITIATOR_ID = 0, PRIMARY_PARTY_ID = 0;
		long long SL = 0;

		rec->get("ORDERID", ORDERID);
		rec->get("CHANNEL", CHANNEL);
		rec->get("ORIGCONVERSATIONID", ORIGCONVERSATIONID);
		rec->get("LINKEDTYPE", LINKEDTYPE);
		rec->get("LINKEDORDERID", LINKEDORDERID);

		rec->get("REQUESTER_ID", REQUESTER_ID);
		rec->get("REQUESTER_TYPE", REQUESTER_TYPE);
		rec->get("REQUESTER_IDENTIFIER_TYPE", REQUESTER_IDENTIFIER_TYPE);
		rec->get("REQUESTER_IDENTIFIER_VALUE", REQUESTER_IDENTIFIER_VALUE);
		rec->get("REQUESTER_MNEMORIC", REQUESTER_MNEMORIC);

		if ((size = rec->get("INITIATOR_ID", buffer, 8)) > 0) INITIATOR_ID = axon::util::bytestoull(buffer, size);
		rec->get("INITIATOR_TYPE", INITIATOR_TYPE);
		rec->get("INITIATOR_IDENTIFIER_TYPE", INITIATOR_IDENTIFIER_TYPE);
		rec->get("INITIATOR_IDENTIFIER_VALUE", INITIATOR_IDENTIFIER_VALUE);
		rec->get("INITIATOR_MNEMONIC", INITIATOR_MNEMONIC);
		// INITIATOR_ORG_SHORTCODE

		if ((size = rec->get("PRIMARY_PARTY_ID", buffer, 8)) > 0) PRIMARY_PARTY_ID = axon::util::bytestoull(buffer, size);
		rec->get("PRIMARY_PARTY_TYPE", PRIMARY_PARTY_TYPE);

		rec->get("THIRDPARTYID", THIRDPARTYID);
		rec->get("THIRDPARTYIP", THIRDPARTYIP);
		rec->get("ACCESSPOINTIP", ACCESSPOINTIP);
		rec->get("THIRDPARTYREQTIME", THIRDPARTYREQTIME);
		rec->get("ACCESSPOINTREQTIME", ACCESSPOINTREQTIME);

		rec->get("ERRORCODE", ERRORCODE);
		rec->get("ERRORMESSAGE", ERRORMESSAGE);

		const char stmt[] = "UPDATE CPS_TRANSACTION_NORMALIZED SET CHANNEL = :aa, ORIGCONVERSATIONID = :ab, LINKEDTYPE = :ac, LINKEDORDERID = :ad, REQUESTER_TYPE = :ae, REQUESTER_IDENTIFIER_TYPE = :af, REQUESTER_IDENTIFIER_VALUE = :ag, REQUESTER_MNEMORIC = :ah, REQUESTER_ID = :ai, SL = :aj WHERE ORDERID = :ak";

		(*db)<<CHANNEL<<ORIGCONVERSATIONID<<LINKEDTYPE<<LINKEDORDERID<<REQUESTER_TYPE<<REQUESTER_IDENTIFIER_TYPE<<REQUESTER_IDENTIFIER_VALUE<<REQUESTER_MNEMORIC<<REQUESTER_ID<<SL<<ORDERID;
		db->execute(stmt);
	}
	else if (table == "CPS_ORDER_REFDATA-VALUE.CPS_ORDER_REFDATA")
	{
		rec->get("ORDERID", ORDERID);
		rec->get("REFERENCE_KEY", REFERENCE_KEY);
		rec->get("REFERENCE_VALUE", REFERENCE_VALUE);

		std::replace(REFERENCE_KEY.begin(), REFERENCE_KEY.end(), ' ', '_');
		std::replace(REFERENCE_KEY.begin(), REFERENCE_KEY.end(), '.', '_');

		std::string stmt = "UPDATE cps_transaction_normalized SET RK_" + REFERENCE_KEY + " = :aa WHERE ORDERID = :oi";

		try {
			db->execute("ALTER TABLE cps_transaction_normalized ADD RK_" + REFERENCE_KEY + " TEXT;");

			(*db)<<REFERENCE_VALUE<<ORDERID;
			db->execute(stmt);
		} catch(axon::exception *e) {
			std::cout<<e->what()<<std::endl<<stmt<<std::endl;
		}
	}

	// std::cout<<axon::timer::iso8601()<<">>"<<ORDERID<<" ("<<op_type<<") - "<<INITIATOR_MNEMONIC<<std::endl;

	count++;
}

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], char* env[])
{
	std::string address, username, password, keyspace, bootstrap, schema, consumer;

	for(int i=0;env[i]!=NULL;i++)
	{
		auto parts = axon::util::split(env[i], '=');

		if (parts[0] == "AXON_USERNAME")
			username = parts[1];
		else if (parts[0] == "AXON_PASSWORD")
			password = parts[1];
		else if (parts[0] == "AXON_HOSTNAME")
			address = parts[1];
		else if (parts[0] == "AXON_KEYSPACE")
			keyspace = parts[1];
		else if (parts[0] == "AXON_BOOTSTRAP")
			bootstrap = parts[1];
		else if (parts[0] == "AXON_SCHEMA_REGISTRY")
			schema = parts[1];
		else if (parts[0] == "AXON_CONSUMER_GROUP")
			consumer = parts[1];
	}

	// if (argc <= 2) return 0;

	axon::stream::kafka source(bootstrap, schema, "axon_trx");
	std::thread th(counter, &source);

	source.add("CPS_ORDERHIS");
	source.add("CPS_ORDER_REFDATA");
	source.add("CPS_TRANS_RECORD");
	source.subscribe();

	signal(SIGINT, stop);

	std::shared_ptr<axon::stream::recordset> rc;
	axon::database::scylladb db;

	db[AXON_DATABASE_KEYSPACE] = keyspace;
	db.connect(address, username, password);

	// std::shared_ptr<axon::database::tableinfo> inf = db.getinfo("cps_transaction_normalized");
	// std::cout<<">> "<<inf->column_exists("credit_party_id")<<std::endl;

	while (running && (rc = std::move(source.next())))
	{
		if (rc->is_empty())
			continue;
		parse(rc, &db);
		// rc->print();
	}

	th.join();

	return 0;
}