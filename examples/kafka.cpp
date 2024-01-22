#include <iostream>
#include <variant>
#include <bitset>
#include <cmath>

#include <axon.h>
#include <axon/kafka.h>
#include <axon/util.h>

#include <signal.h>

extern "C" {
	#include <avro.h>
	#include <libserdes/serdes.h>
	#include <libserdes/serdes-avro.h>
}

#include <cassandra.h>
#include <dse.h>

axon::transport::transfer::kafka c("host1,host2,host3", "schema-reg1,schema-reg2,schema-reg3", "KafkaConsumer");

typedef std::variant<std::nullptr_t, std::string, double, int64_t, int, bool> avt;

CassFuture* connect_future = NULL;
CassCluster* cluster = NULL;
CassSession* session = NULL;

static void stop (int sig)
{
	c.stop();
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);

	cass_future_free(connect_future);
	cass_cluster_free(cluster);
	cass_session_free(session);
}

unsigned long long bytestoull(const char *bcd, const size_t size)
{
	std::stringstream ss;

	for (size_t i = 0; i < size; i++)
	{
		std::bitset<8> x(bcd[i]);
		ss<<x;
	}
	std::bitset<64> bits(ss.str());
	
	return bits.to_ullong();
}

std::string bytestodecstring(const char *bcd, const size_t size)
{
	std::stringstream ss;

	for (size_t i = 0; i < size; i++)
	{
		std::bitset<8> x(bcd[i]);
		ss<<x;
	}

	return ss.str();
}

void dbg(avro_value_t *avro)
{
	char *as_json;
	
	if (avro_value_to_json(avro, 1, &as_json))
		fprintf(stderr, "=> avro_to_json failed: %s\n", avro_strerror());
	else {
		printf("%15s\n", as_json);
		free(as_json);
	}
}

avt xget(avro_value_t *avro, std::string name)
{
	avro_value_t init_iden_value;
	avt retval;

	if (avro_value_get_by_name(avro, name.c_str(), &init_iden_value, NULL) == 0)
	{
		avro_value_t branch;

		// union_schema = avro_value_get_schema(&init_iden_value);
		avro_value_set_branch(&init_iden_value, 1, &branch);

		switch (avro_value_get_type(&branch))
		{
			case AVRO_BOOLEAN:
			{
				int value;

				avro_value_get_boolean(&branch, &value);
				return (value==0)?false:true;
			}
			break;

			case AVRO_BYTES:
			{
				const void *buffer;
				size_t  size;

				avro_value_get_bytes(&branch, &buffer, &size);

				return reinterpret_cast<const char*>(buffer);
			}
			break;

			case AVRO_STRING:
			{
				const char *buffer;
				size_t size;

				avro_value_get_string(&branch, &buffer, &size);
				return buffer;
			}
			break;

			case AVRO_INT64:
			{
				int64_t value;

            	avro_value_get_long(&branch, &value);
				return value;
			}
			break;

			default:
				std::cout<<"unknown avro type: "<<avro_value_get_type(&branch)<<std::endl;
				return nullptr;
				break;
		}
		// std::cout<<name<<disc<<" ("<<avro_value_get_type(&branch)<<"): "<<p<<std::endl;
	}

	return nullptr;
}

size_t get(avro_value_t *avro, std::string name, void *value, size_t size)
{
	avro_value_t init_iden_value;

	if (avro_value_get_by_name(avro, name.c_str(), &init_iden_value, NULL) == 0)
	{
		avro_value_t branch;

		avro_value_set_branch(&init_iden_value, 1, &branch);

		if (avro_value_get_type(&branch) == AVRO_BYTES)
		{
			const void *buffer;
			size_t bsz = 0;

			avro_value_get_bytes(&branch, &buffer, &bsz);

			if (bsz <= 0)
				return 0;

			memcpy(value, buffer, std::min(size, bsz));

			return std::min(size, bsz);
		}
	}

	return 0;
}

size_t get(avro_value_t *avro, std::string name, std::string &value)
{
	avro_value_t init_iden_value;

	if (avro_value_get_by_name(avro, name.c_str(), &init_iden_value, NULL) == 0)
	{
		avro_value_t branch;

		avro_value_set_branch(&init_iden_value, 1, &branch);

		if (avro_value_get_type(&branch) == AVRO_STRING)
		{
			const char *buffer;
			size_t size;

			avro_value_get_string(&branch, &buffer, &size);

			if (size <= 0)
				return 0;
			
			value = buffer;

			return size;
		}
	}

	return 0;
}

bool get(avro_value_t *avro, std::string name, int64_t &value)
{
	avro_value_t init_iden_value;

	if (avro_value_get_by_name(avro, name.c_str(), &init_iden_value, NULL) == 0)
	{
		avro_value_t branch;

		avro_value_set_branch(&init_iden_value, 1, &branch);

		if (avro_value_get_type(&branch) == AVRO_INT64)
		{
			avro_value_get_long(&branch, &value);
			return true;
		}
	}

	return false;
}

void parse(avro_value_t *avro)
{
	avt retval;
	
	// dbg(avro);
	std::string table, op_type;

	get(avro, "table", table);
	get(avro, "op_type", op_type);
	// std::cout<<table<<"$"<<op_type;

	try {

		if (table == "CPS_TRANS_RECORD-VALUE.CPS_TRANS_RECORD")
		{
			std::string ORDERID, TRANS_STATUS, DEBIT_PARTY_TYPE, DEBIT_PARTY_MNEMONIC, CREDIT_PARTY_TYPE, CREDIT_PARTY_MNEMONIC, REQUEST_CURRENCY, ACCOUNT_UNIT_TYPE, CURRENCY, IS_REVERSED, IS_PARTIAL_REVERSED, IS_REVERSING, REMARK, BANK_ACCOUNT_NUMBER, BANK_ACCOUNT_NAME, CONSUMED_BUNDLE;
			long TRANS_INITATE_TIME, TRANS_END_TIME, EXPIRED_TIME, LAST_UPDATED_TIME, REQUEST_AMOUNT, ORG_AMOUNT, ACTUAL_AMOUNT, FEE, COMMISSION, TAX, DISCOUNT_AMOUNT, REDEEMED_POINT_AMOUNT;
			unsigned long long DEBIT_PARTY_ID = 0, CREDIT_PARTY_ID = 0, REASON_TYPE = 0, EXCHANGE_RATE = 0;
			char buffer[16] = { 0 };
			size_t size;

			get(avro, "ORDERID", ORDERID);
			get(avro, "TRANS_STATUS", TRANS_STATUS);
			get(avro, "DEBIT_PARTY_TYPE", DEBIT_PARTY_TYPE);
			get(avro, "DEBIT_PARTY_MNEMONIC", DEBIT_PARTY_MNEMONIC);
			get(avro, "CREDIT_PARTY_TYPE", CREDIT_PARTY_TYPE);
			get(avro, "CREDIT_PARTY_MNEMONIC", CREDIT_PARTY_MNEMONIC);
			get(avro, "REQUEST_CURRENCY", REQUEST_CURRENCY);
			get(avro, "ACCOUNT_UNIT_TYPE", ACCOUNT_UNIT_TYPE);
			get(avro, "CURRENCY", CURRENCY);
			get(avro, "REMARK", REMARK);
			get(avro, "IS_REVERSED", IS_REVERSED);
			get(avro, "IS_PARTIAL_REVERSED", IS_PARTIAL_REVERSED);
			get(avro, "IS_REVERSING", IS_REVERSING);
			get(avro, "BANK_ACCOUNT_NUMBER", BANK_ACCOUNT_NUMBER);
			get(avro, "BANK_ACCOUNT_NAME", BANK_ACCOUNT_NAME);
			get(avro, "CONSUMED_BUNDLE", CONSUMED_BUNDLE);
			

			get(avro, "TRANS_INITATE_TIME", TRANS_INITATE_TIME);
			get(avro, "TRANS_END_TIME", TRANS_END_TIME);
			get(avro, "EXPIRED_TIME", EXPIRED_TIME);
			get(avro, "LAST_UPDATED_TIME", LAST_UPDATED_TIME);
			get(avro, "REQUEST_AMOUNT", REQUEST_AMOUNT);
			get(avro, "ORG_AMOUNT", ORG_AMOUNT);
			get(avro, "ACTUAL_AMOUNT", ACTUAL_AMOUNT);
			get(avro, "FEE", FEE);
			get(avro, "COMMISSION", COMMISSION);
			get(avro, "TAX", TAX);
			get(avro, "DISCOUNT_AMOUNT", DISCOUNT_AMOUNT);
			get(avro, "REDEEMED_POINT_AMOUNT", REDEEMED_POINT_AMOUNT);
			
			if ((size = get(avro, "DEBIT_PARTY_ID", buffer, 8)) > 0) DEBIT_PARTY_ID = bytestoull(buffer, size);
			if ((size = get(avro, "CREDIT_PARTY_ID", buffer, 8)) > 0) CREDIT_PARTY_ID = bytestoull(buffer, size);
			if ((size = get(avro, "REASON_TYPE", buffer, 8)) > 0) REASON_TYPE = bytestoull(buffer, size);
			if ((size = get(avro, "EXCHANGE_RATE", buffer, 8)) > 0) EXCHANGE_RATE = bytestoull(buffer, size);

			// CHECKER_ID
			// BANK_CARD_ID
			// FI_ACCOUNT_INFO
			// REDEEMED_POINT_TYPE

			std::stringstream cqlstmt;

			cqlstmt<<"UPDATE CPS_TRANSACTION_NORMALIZED SET ";
			cqlstmt<<"TRANS_STATUS = '"<<TRANS_STATUS<<"'";
			cqlstmt<<", DEBIT_PARTY_TYPE = '"<<DEBIT_PARTY_TYPE<<"'";
			cqlstmt<<", DEBIT_PARTY_MNEMONIC = '"<<DEBIT_PARTY_MNEMONIC<<"'";
			cqlstmt<<", CREDIT_PARTY_TYPE = '"<<CREDIT_PARTY_TYPE<<"'";
			cqlstmt<<", CREDIT_PARTY_MNEMONIC = '"<<CREDIT_PARTY_MNEMONIC<<"'";
			cqlstmt<<", REQUEST_CURRENCY = '"<<REQUEST_CURRENCY<<"'";
			cqlstmt<<", ACCOUNT_UNIT_TYPE = '"<<ACCOUNT_UNIT_TYPE<<"'";
			cqlstmt<<", CURRENCY = '"<<CURRENCY<<"'";
			cqlstmt<<", REMARK = '"<<REMARK<<"'";
			cqlstmt<<", IS_REVERSED = '"<<IS_REVERSED<<"'";
			cqlstmt<<", IS_PARTIAL_REVERSED = '"<<IS_PARTIAL_REVERSED<<"'";
			cqlstmt<<", IS_REVERSING = '"<<IS_REVERSING<<"'";
			cqlstmt<<", BANK_ACCOUNT_NUMBER = '"<<BANK_ACCOUNT_NUMBER<<"'";
			cqlstmt<<", BANK_ACCOUNT_NAME = '"<<BANK_ACCOUNT_NAME<<"'";
			cqlstmt<<", CONSUMED_BUNDLE = '"<<CONSUMED_BUNDLE<<"'";
			cqlstmt<<", TRANS_INITATE_TIME = "<<TRANS_INITATE_TIME;
			cqlstmt<<", TRANS_END_TIME = "<<TRANS_END_TIME;
			cqlstmt<<", EXPIRED_TIME = "<<EXPIRED_TIME;
			cqlstmt<<", LAST_UPDATED_TIME = "<<LAST_UPDATED_TIME;
			cqlstmt<<", REQUEST_AMOUNT = "<<REQUEST_AMOUNT;
			cqlstmt<<", ORG_AMOUNT = "<<ORG_AMOUNT;
			cqlstmt<<", ACTUAL_AMOUNT = "<<ACTUAL_AMOUNT;
			cqlstmt<<", FEE = "<<FEE;
			cqlstmt<<", COMMISSION = "<<COMMISSION;
			cqlstmt<<", TAX = "<<TAX;
			cqlstmt<<", DISCOUNT_AMOUNT = "<<DISCOUNT_AMOUNT;
			cqlstmt<<", REDEEMED_POINT_AMOUNT = "<<REDEEMED_POINT_AMOUNT;
			cqlstmt<<", DEBIT_PARTY_ID = "<<DEBIT_PARTY_ID;
			cqlstmt<<", CREDIT_PARTY_ID = "<<CREDIT_PARTY_ID;
			cqlstmt<<", REASON_TYPE = "<<REASON_TYPE;
			cqlstmt<<", EXCHANGE_RATE = "<<EXCHANGE_RATE;
			cqlstmt<<" WHERE ORDERID = '"<<ORDERID<<"'";

			CassStatement* statement = cass_statement_new(cqlstmt.str().c_str(), 0);
			CassError rc = CASS_OK;
			CassFuture* query_future = cass_session_execute(session, statement);
			
			cass_statement_free(statement);
			rc = cass_future_error_code(query_future);
			if (rc != CASS_OK) printf("Query result: %s ===>> %s\n", cass_error_desc(rc), cqlstmt.str().c_str());
			cass_future_free(query_future);

			std::cout<<"OrderID = "<<ORDERID<<std::endl;
			std::cout<<"CPS_TRANS_RECORD => "<<cqlstmt.str()<<std::endl;
		}
		else if (table == "CPS_ORDERHIS-VALUE.CPS_ORDERHIS")
		{
			std::string ORDERID, CHANNEL, ORIGCONVERSATIONID, LINKEDTYPE, LINKEDORDERID;
			unsigned long long REQUESTER_ID, SL;
			char buffer[16] = { 0 };
			long ENDTIME;
			size_t size;

			get(avro, "ORDERID", ORDERID);
			get(avro, "CHANNEL", CHANNEL);
			get(avro, "ORIGCONVERSATIONID", ORIGCONVERSATIONID);
			get(avro, "LINKEDTYPE", LINKEDTYPE);
			get(avro, "LINKEDORDERID", LINKEDORDERID);

			get(avro, "ENDTIME", ENDTIME);

			if ((size = get(avro, "REQUESTER_ID", buffer, 8)) > 0) REQUESTER_ID = bytestoull(buffer, size);
			if ((size = get(avro, "SL", buffer, 8)) > 0) SL = bytestoull(buffer, size);

			std::stringstream cqlstmt;

			cqlstmt<<"UPDATE CPS_TRANSACTION_NORMALIZED SET ";
			cqlstmt<<"CHANNEL = '"<<CHANNEL<<"',";
			cqlstmt<<"ORIGCONVERSATIONID = '"<<ORIGCONVERSATIONID<<"',";
			cqlstmt<<"LINKEDTYPE = '"<<LINKEDTYPE<<"',";
			cqlstmt<<"LINKEDORDERID = '"<<LINKEDORDERID<<"'";
			cqlstmt<<", ENDTIME = "<<ENDTIME;
			cqlstmt<<", REQUESTER_ID = "<<REQUESTER_ID;
			cqlstmt<<", SL = "<<SL;
			cqlstmt<<" WHERE ORDERID = '"<<ORDERID<<"'";

			CassStatement* statement = cass_statement_new(cqlstmt.str().c_str(), 0);
			CassError rc = CASS_OK;
			CassFuture* query_future = cass_session_execute(session, statement);
			
			cass_statement_free(statement);
			rc = cass_future_error_code(query_future);
			if (rc != CASS_OK) printf("Query result: %s ===>> %s\n", cass_error_desc(rc), cqlstmt.str().c_str());
			cass_future_free(query_future);
			std::cout<<"CPS_ORDERHIS => "<<cqlstmt.str()<<std::endl;
		}
		else if (table == "CPS_ORDER_REFDATA-VALUE.CPS_ORDER_REFDATA")
		{
			std::string ORDERID, REFERENCE_KEY, REFERENCE_VALUE;

			get(avro, "ORDERID", ORDERID);
			get(avro, "REFERENCE_KEY", REFERENCE_KEY);
			get(avro, "REFERENCE_VALUE", REFERENCE_VALUE);

			// std::cout<<"%%%"<<ORDERID<<"<>"<<REFERENCE_KEY<<"<>"<<REFERENCE_VALUE<<std::endl;
		}
	} catch (std::exception &e) {
		std::cout<<"error-> "<<e.what()<<std::endl;
	}
}

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], char* env[])
{
	std::string address, username, password, schema;

	for(int i=0;env[i]!=NULL;i++)
	{
    	auto parts = axon::util::split(env[i], '=');

		if (parts[0] == "AXON_USERNAME")
			username = parts[1];
		else if (parts[0] == "AXON_PASSWORD")
			password = parts[1];
		else if (parts[0] == "AXON_ADDRESS")
			address = parts[1];
		else if (parts[0] == "AXON_SCHEMA")
			schema = parts[1];
	}

	cluster = cass_cluster_new();
	session = cass_session_new();

	cass_cluster_set_contact_points(cluster, address.c_str());
	cass_cluster_set_core_connections_per_host(cluster, 2);
	cass_cluster_set_dse_plaintext_authenticator(cluster, username.c_str(), password.c_str());

	connect_future = cass_session_connect_keyspace(session, cluster, schema.c_str());

	CassError rc = cass_future_error_code(connect_future);

	if (rc != CASS_OK) {
		/* Display connection error message */
		const char* message;
		size_t message_length;
		cass_future_error_message(connect_future, &message, &message_length);
		fprintf(stderr, "Connect error: '%.*s'\n", (int)message_length, message);
		return -1;
	}

	c.add("CPS_ORDERHIS");
	c.add("CPS_ORDER_REFDATA");
	c.add("CPS_TRANS_RECORD");

	c.connect();

	c.start(&parse);
	
	signal(SIGINT, stop);
	
	// std::cout<<"return value is: "<<tcn::version()<<std::endl;

	return 0;
}
