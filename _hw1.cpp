#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <iterator>
#include <iomanip>
#include <ctime>

using namespace std;

struct Transaction {
	vector<int> itemIds;
};

struct ItemsetInfo {
	vector<int> itemIds;
	vector<int> appearancesInTransactions; // transaction id, where this itemset appears
};

vector<Transaction> Read_Input(char* input_path, map<int, ItemsetInfo>& initial_itemsetInfo_map) {
	vector<Transaction> transactions;

	ifstream input_file(input_path);

	int rowId = 0;
	for (string row; getline(input_file, row); ) {
		Transaction transaction { vector<int>() };
		
		istringstream iss(row);
		int item_id;
		char comma;

		iss >> item_id;
		transaction.itemIds.push_back(item_id);

		if (initial_itemsetInfo_map[item_id].itemIds.size() == 0) 
			initial_itemsetInfo_map[item_id].itemIds.push_back(item_id);
		initial_itemsetInfo_map[item_id].appearancesInTransactions.push_back(rowId);

		while ((iss >> comma >> item_id)) {
			transaction.itemIds.push_back(item_id);
			if (initial_itemsetInfo_map[item_id].itemIds.size() == 0)
				initial_itemsetInfo_map[item_id].itemIds.push_back(item_id);
			initial_itemsetInfo_map[item_id].appearancesInTransactions.push_back(rowId);
		}
		
		// check if sorted

		transactions.push_back(transaction);
		rowId++;
	}

	return transactions;
}

bool _Next_Itemset_Couple(int& set1_id, int& set2_id, int vector_size) {
	set2_id++;
	if (set2_id >= vector_size) {
		set1_id += 1;
		set2_id = set1_id + 1;
	}
	if (set2_id >= vector_size || set1_id >= vector_size) return false;
	return true;
}


vector<int> _Join_Itemset_If_Able(ItemsetInfo& set1, ItemsetInfo& set2) {
	vector<int> empty_vector;
	if (set1.itemIds.size() != set2.itemIds.size()) return empty_vector; // unable

	set<int> item_union_set;
	for (int id : set1.itemIds) item_union_set.insert(id);
	for (int id : set2.itemIds) item_union_set.insert(id);
	if (item_union_set.size() != set1.itemIds.size() + 1) return empty_vector; // unable

	return vector<int>(item_union_set.begin(), item_union_set.end());
}


ItemsetInfo _Join_ItemsetInfo_If_Able(ItemsetInfo& set1, ItemsetInfo& set2) {
	vector<int> joined_itemset = _Join_Itemset_If_Able(set1, set2);
	if (joined_itemset.empty()) return {}; // unable

	set<int> appearance_intersect;
	set_intersection(set1.appearancesInTransactions.begin(), set1.appearancesInTransactions.end(),
		set2.appearancesInTransactions.begin(), set2.appearancesInTransactions.end(),
		inserter(appearance_intersect, appearance_intersect.begin()));

	return { joined_itemset, vector<int>(appearance_intersect.begin(), appearance_intersect.end()) };
}

vector< vector<ItemsetInfo> > Apriori(int min_support_count, vector<ItemsetInfo> initial_itemsetInfo_vector, vector<Transaction> transactions) {
	vector< vector<ItemsetInfo> > n_item_itemsetInfo_vectors;
	n_item_itemsetInfo_vectors.push_back(initial_itemsetInfo_vector);

	int source_index = 0;
	while (true) {
		vector<ItemsetInfo>& source_itemsetInfo_vector = n_item_itemsetInfo_vectors[source_index];
		
		vector<ItemsetInfo> new_item_itemsetInfo_vector;
		set<vector<int>> new_itemIds;

		int set1_id = 0, set2_id = 0;
		while (_Next_Itemset_Couple(set1_id, set2_id, source_itemsetInfo_vector.size())) {
			ItemsetInfo candidate_itemset = _Join_ItemsetInfo_If_Able(source_itemsetInfo_vector[set1_id], source_itemsetInfo_vector[set2_id]);
			if ( candidate_itemset.itemIds.size() != 0 ) {
				if (candidate_itemset.appearancesInTransactions.size() >= min_support_count) {
					if (new_itemIds.find(candidate_itemset.itemIds) == new_itemIds.end()) {
						new_item_itemsetInfo_vector.push_back(candidate_itemset);
						new_itemIds.insert(candidate_itemset.itemIds);
					}
				}
			}
		}

		if (new_item_itemsetInfo_vector.size() == 0) {
			break;
		}
		else {
			n_item_itemsetInfo_vectors.push_back(new_item_itemsetInfo_vector);
		}

		source_index += 1;
	}

	return n_item_itemsetInfo_vectors;
}


void Output_Frequent_Pattern(vector< vector<ItemsetInfo> > n_item_itemsetInfo_vectors, int n_transaction, char* output_file_name) {
	ofstream fout(output_file_name);

	fout.setf(ios::fixed);
	fout << setprecision(4);

	for (vector<ItemsetInfo>& itemsetInfo_vector : n_item_itemsetInfo_vectors) {
		for (ItemsetInfo itemsetInfo : itemsetInfo_vector) {
			fout << itemsetInfo.itemIds[0];
			for (int i = 1; i < itemsetInfo.itemIds.size(); i++) {
				fout << "," << itemsetInfo.itemIds[i];
			}
			float support = 1.0 * itemsetInfo.appearancesInTransactions.size() / n_transaction;
			fout << ":" << ((float)((int)(support * 10000 + 0.5))) / 10000 << endl;
		}
	}
	fout.close();
}


// argv: 程式路徑 min_support 輸入檔名 輸出檔名
int main(int argc, char** argv) {
	clock_t tStart = clock();

	map<int, ItemsetInfo> initial_itemsetInfo_map;
	vector<ItemsetInfo> initial_itemsetInfo_vector;
	
	// 讀取
	vector<Transaction> transactions = Read_Input(argv[2], initial_itemsetInfo_map);
	float min_support = stof(argv[1]);
	int min_support_count = 0.5 + min_support * transactions.size();

	for (map<int, ItemsetInfo>::iterator it = initial_itemsetInfo_map.begin(); it != initial_itemsetInfo_map.end(); it++) {
		if (it->second.appearancesInTransactions.size() >= min_support_count)
			initial_itemsetInfo_vector.push_back(it->second);
	}

	// 計算
	vector< vector<ItemsetInfo> > n_item_itemsetInfo_vectors = Apriori(min_support_count, initial_itemsetInfo_vector, transactions);

	// 輸出
	Output_Frequent_Pattern(n_item_itemsetInfo_vectors, transactions.size(), argv[3]);


	cout << "Time:" << 1.0*(clock() - tStart)/CLOCKS_PER_SEC << endl;
	return 0;
}
