#include "Document.h"
#include <sdb/SequentialDB.h>

using namespace sf1v5;
using namespace izenelib::am;

bool isDump = false;
int num = 1000000;
static string sdb_type = "btree";

string
		filename =
				"/home/wps/codebase/sf1-r/sf1-revolution/bin/collection/index-data/DocumentPropertyTable.dat";

sdb_btree<docid_t, Document> sdb(filename);
sdb_btree<docid_t, Document> sdb0("_bt.dat");
sdb_bptree<docid_t, Document> sdb1("_bp.dat");
sdb_storage<docid_t, Document> sdb2("_seq.dat");

template<typename T1, typename T2> void dump(T1& t1, T2& t2, int num=100000) {
	if ( !t1.is_open() )
		t1.open();
	if ( !t2.is_open() )
		t2.open();

	typename T1::SDBCursor locn = t1.get_first_locn();
	docid_t key;
	Document value;
	int count = 0;
	while (t1.get(locn, key, value)) {
		t2.insert(key, value);
		count++;
		if (count%10000 == 0)
			cout<<"idx: "<<count<<endl;
		if (count > num)
			break;
		if ( !t1.seq(locn) )
			break;
	}
	t2.close();
}

template<typename SDB> void query_test(SDB& sdb) {
	sdb.open();
	Document doc;

	izenelib::util::ClockTimer timer;
	double worst = 0.0;
	int times = 1000;
	for (int i=0; i<times; i++) {
		izenelib::util::ClockTimer timer1;
		
		set<int> input;
		set<int>::iterator it;
		int num = sdb.num_items()+1;
		for (int j=0; j<20; j++)
			input.insert(rand()%num);
		for (it=input.begin(); it != input.end(); it++) {
			//			cout<<*it<<endl;
			sdb.get(*it, doc);
		}
		double elapsed = timer1.elapsed();
		printf("one query elapsed: ( actually ): %lf seconds\n",  elapsed);
		if( elapsed > worst )
			worst = elapsed;
		
		if ( i % 100 == 0 ) {
			izene_serialization<Document> izs(doc);
			char* str;
			size_t sz;
			izs.write_image(str, sz);
			cout<<"doc sz:"<<sz<<endl;
		}
	}
	printf("average elapsed 1 ( actually ): %lf seconds\n, worse: %lf seconds", timer.elapsed()/times, worst );
}

template<typename SDB> void process(SDB& db) {
	if (isDump)
		dump(sdb, db, num);
	else
		query_test(db);
}

void initialize() {
	sdb0.setDegree(12);
	//sdb0.setPageSize(8192*8);
}

int main(int argc, char* argv[]) {
	initialize();
	if (argv[1]) {
		isDump = atoi(argv[1]);
	}
	if (argv[2])
		sdb_type = argv[2];
	if (argv[3])
		num = atoi(argv[3]);

	if (sdb_type == "ori") {
		isDump = false;
		process(sdb);
	}
	if (sdb_type == "btree")
		process(sdb0);
	else if (sdb_type == "bptree")
		process(sdb1);
	else if (sdb_type == "seq") {
		process(sdb2);
	} else {
		process(sdb0);
		process(sdb1);
		process(sdb2);
		process(sdb);
	}

	return 1;
}
