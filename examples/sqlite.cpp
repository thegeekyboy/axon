#include <axon.h>
#include <axon/database.h>

int main()
{
	long total = 100000;

	srand(time(NULL));

	try
	{
		axon::database::sqlite db("abcd.dbf");
		int i = 20;

		try {

			db.execute("CREATE TABLE BLAH (X INT, Y TEXT);");
			db.execute("CREATE INDEX IXD_BLAH_X ON BLAH(X);");

		} catch (axon::exception& e) {
			std::cout<<">"<<e.what()<<std::endl;
		}

		db.execute("BEGIN TRANSACTION;");
		for (long x = 0; x < total; x++)
		{
			std::string test = "INSERT INTO BLAH VALUES(" + std::to_string(rand() % 500000 + 1) + ", 'TESTING');";
			db.execute(test);
		}
		db.execute("END TRANSACTION;");
		
		db.query("SELECT * FROM BLAH WHERE X < @FN");
		db<<i;
		i = 100;
		std::string blah;

		while (db.next())
		{
			db>>i>>blah;
			std::cout<<"ID "<<i<<" = "<<blah<<std::endl;
		}
		db.done();

		db.execute("DROP TABLE BLAH;");

	} catch (axon::exception& e) {

		std::cout<<e.what()<<std::endl;
	}

	return 0;
}