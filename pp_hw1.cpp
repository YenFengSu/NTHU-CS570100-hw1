#include <algorithm>
#include <utility>
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
#include <omp.h>
#include <chrono>
#include <thread>

using namespace std;

struct Transaction {
	vector<int> itemIds;
};

struct ItemsetInfo {
	vector<int> itemIds;
	vector<int> appearancesInTransactions; // transaction id, where this itemset appears
};

int Read_Input(char* input_path, map<int, ItemsetInfo>& initial_itemsetInfo_map) {
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

	return transactions.size();
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

//// This uses too much memory 
// pair<vector<pair<int,int>>::const_iterator, vector<pair<int,int>>::const_iterator> Delegate_Set_Id_Pair(vector<pair<int, int>>& set_id_pairs, int n_thread, int& pair_progress_id) {
// 	// string dbg_string;
// 	// dbg_string = "n_thread=" + to_string(n_thread) + "set size=" + to_string(set_id_pairs.size()) + "\n";
// 	// cout << dbg_string;
// 	if (pair_progress_id >= set_id_pairs.size()) return {set_id_pairs.end(), set_id_pairs.end()};
// 	const int MIN_PAIR_COUNT = 4;
// 	int pair_count = set_id_pairs.size() / (n_thread * n_thread);
// 	if (pair_count < MIN_PAIR_COUNT) pair_count = MIN_PAIR_COUNT;
// 	if (pair_progress_id + pair_count >= set_id_pairs.size())  pair_count = set_id_pairs.size() - pair_progress_id;
// 	// dbg_string = "pair_count=" + to_string(pair_count) + "\n";
// 	// cout << dbg_string;

// 	vector<pair<int,int>>::const_iterator first = set_id_pairs.begin() + pair_progress_id;
// 	vector<pair<int,int>>::const_iterator last = set_id_pairs.begin() + pair_progress_id + pair_count;
// 	pair_progress_id += pair_count;
// 	return {first, last};
// }

bool Delegate_Set_Start_End(
        pair<int,int>& set_id_pair_progress,
        pair<int,int>& start_id_pair,
        pair<int,int>& end_id_pair,
        int preferred_set_count, int source_itemset_count
){
	if (set_id_pair_progress.first >= source_itemset_count || set_id_pair_progress.second >= source_itemset_count) return false;
	start_id_pair = set_id_pair_progress;
    const int MIN_PAIR_COUNT = 4;
	int pair_count = preferred_set_count;
	if (pair_count < MIN_PAIR_COUNT) pair_count = MIN_PAIR_COUNT;

    int available_pair_count = 0;
    int end_set1_id = start_id_pair.first;
    int end_set2_id = start_id_pair.second;
    while(_Next_Itemset_Couple(end_set1_id, end_set2_id, source_itemset_count)) {
        available_pair_count++;
        if (available_pair_count >= pair_count) break;
    }
    end_id_pair.first = end_set1_id;
    end_id_pair.second = end_set2_id;
	set_id_pair_progress = {end_set1_id, end_set2_id};
    return true;
}

vector< vector<ItemsetInfo> > Apriori(int min_support_count, vector<ItemsetInfo> initial_itemsetInfo_vector) {
	vector< vector<ItemsetInfo> > n_item_itemsetInfo_vectors;
	n_item_itemsetInfo_vectors.push_back(initial_itemsetInfo_vector);

	int source_index = 0;
	while (true) {
		vector<ItemsetInfo>& source_itemsetInfo_vector = n_item_itemsetInfo_vectors[source_index];
		
		vector<ItemsetInfo> new_item_itemsetInfo_vector;

		// List available pairs to test join-ablility
		// cout << "\n\nListing avaliable pairs" << endl;
		// vector<pair<int, int>> set_id_pairs;
		// int pset1_id = 0, pset2_id = 0;
		// while (_Next_Itemset_Couple(pset1_id, pset2_id, source_itemsetInfo_vector.size())) {
		// 	set_id_pairs.push_back({pset1_id, pset2_id});
		// 	// cout << "(" << pset1_id << ", " << pset2_id << ")" << endl;
		// }
		// cout << "pairs size=" << to_string(set_id_pairs.size()) << endl;

		// int pair_progress_id = 0;
		
		map<int, vector<ItemsetInfo>> candidate_new_item_vector;
        pair<int, int> set_id_pair_progress = {0,1}; // TODO: only 1 itemset in source?
        int source_itemset_count = source_itemsetInfo_vector.size();
        int total_combination_count = source_itemsetInfo_vector.size() * (source_itemsetInfo_vector.size() - 1) / 2;

		#pragma omp parallel
		{
            int threadId = omp_get_thread_num();
            int n_thread = omp_get_num_threads();
            int preferred_set_count = total_combination_count / (n_thread * n_thread);
            
            #pragma omp critical
            {
                candidate_new_item_vector[threadId] = vector<ItemsetInfo>();
            }
			vector<ItemsetInfo>& new_item_vector = candidate_new_item_vector[threadId];
			while (true) {

				set<vector<int>> new_itemIds; // used for detecting duplicate
				
				
				// 取得這個 thread 要檢查的 id pair
                pair<int, int> thread_start_set_id_pair, thread_end_set_id_pair;
                bool has_work_to_do;
                // pair<vector<pair<int,int>>::const_iterator, vector<pair<int,int>>::const_iterator> sub_set_iter_range;
				// vector<pair<int,int>> sub_set_id_pair;
				#pragma omp critical
				{
					has_work_to_do = Delegate_Set_Start_End(set_id_pair_progress, thread_start_set_id_pair, thread_end_set_id_pair, preferred_set_count, source_itemset_count);
					// cout << "thread " << threadId << " set id range: " << "(" << thread_start_set_id_pair.first << "," << thread_start_set_id_pair.second << ")" ;
					// cout << " -> " << "(" << thread_end_set_id_pair.first << "," << thread_end_set_id_pair.second << ")" << endl;
				}
				if (!has_work_to_do) {
					// #pragma omp critical
					// {
					// 	cout << "thread " << to_string(threadId) << " finished." << endl;
					// }
					break;
				}

				// debug
				// string dbg_string;
                // dbg_string = "thread: " + to_string(threadId) + " got\n";
				// for (vector<pair<int,int>>::const_iterator it = sub_set_iter_range.first; it != sub_set_iter_range.second; it ++) {
				// 	dbg_string += "\t(" + to_string(it->first) + "," + to_string(it->second) + "),";
				// }
				// dbg_string += "\n";
				// #pragma omp critical
				// {
				// 	cout << dbg_string << endl;
				// }

				// 開始檢查每個 id pair
                int set1_id = thread_start_set_id_pair.first;
                int set2_id = thread_start_set_id_pair.second;
				while (true) {
					// dbg_string = "thread " + to_string(threadId) + " joining ";
					// string pairList = "(";
					// for (int cId = 0; cId < source_itemsetInfo_vector[set1_id].itemIds.size(); cId++) {
					// 	if (cId != 0) pairList += ",";
					// 	pairList += to_string(source_itemsetInfo_vector[set1_id].itemIds[cId]);
					// }
					// pairList += ") , (";
					// for (int cId = 0; cId < source_itemsetInfo_vector[set2_id].itemIds.size(); cId++) {
					// 	if (cId != 0) pairList += ",";
					// 	pairList += to_string(source_itemsetInfo_vector[set2_id].itemIds[cId]);
					// }
					// pairList += ")";
					// dbg_string += pairList + "... ";

					// 測試＆取得 itemset
					ItemsetInfo candidate_itemset = _Join_ItemsetInfo_If_Able(source_itemsetInfo_vector[set1_id], source_itemsetInfo_vector[set2_id]);

					// 判斷是否符合 min support
					if ( candidate_itemset.itemIds.size() != 0 ) {
						// dbg_string += "join-able... ";
						if (candidate_itemset.appearancesInTransactions.size() >= min_support_count) {
							// dbg_string += "exceed minimum support count... ";
							if (new_itemIds.find(candidate_itemset.itemIds) == new_itemIds.end()) {
								// dbg_string += "no duplicate.";
								new_item_vector.push_back(candidate_itemset);
								new_itemIds.insert(candidate_itemset.itemIds);
							}
						}
					}

					// #pragma omp critical
					// {
					// 	cout << dbg_string << endl;
					// }

                    // 下一個 pair
                    if (!_Next_Itemset_Couple(set1_id, set2_id, source_itemset_count)) {
                        break;
                    }
                    if (set1_id >= thread_end_set_id_pair.first && set2_id >= thread_end_set_id_pair.second) {
                        break;
                    }

				}


				
				
			}
			// #pragma omp barrier
			// #pragma omp single
			// {
			// 	for (int tId = 0; tId < omp_get_num_threads(); tId ++) {
			// 		cout << "thread " << tId << " itemsets:" << endl;
			// 		for (int aye = 0; aye < candidate_new_item_vector[tId].size(); aye++) {
			// 			for (int jay = 0; jay < candidate_new_item_vector[tId][aye].itemIds.size(); jay++) {
			// 				cout << candidate_new_item_vector[tId][aye].itemIds[jay] << ",";
			// 			}
			// 			cout << ":" << candidate_new_item_vector[tId][aye].appearancesInTransactions.size() << endl;
			// 		}
			// 	}
			// }
		}

		
		// cout << "\n\n===========Merging===========\n";
		set<vector<int>> itemIdsSet;
		for (map<int, vector<ItemsetInfo>>::iterator it = candidate_new_item_vector.begin(); it != candidate_new_item_vector.end(); it++) {
			int threadId = it->first;
			vector<ItemsetInfo>& thread_itemsetInfo = it->second;

			// cout << "Thread " << threadId << ":\n";
			for (int tisId = 0; tisId < thread_itemsetInfo.size(); tisId++) {

				// cout << "(";
				// for (int isId = 0; isId < thread_itemsetInfo[tisId].itemIds.size(); isId++) {
				// 	if (isId != 0) cout << ",";
				// 	cout << thread_itemsetInfo[tisId].itemIds[isId];
				// }
				// cout << ")...";

				if (itemIdsSet.find(thread_itemsetInfo[tisId].itemIds) == itemIdsSet.end()) {
					itemIdsSet.insert(thread_itemsetInfo[tisId].itemIds);
					new_item_itemsetInfo_vector.push_back(thread_itemsetInfo[tisId]);
					// cout << "New ItemSet" << endl;
				} else {
					// cout << "Duplicate" << endl;
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

void ParaMain(map<int, ItemsetInfo>& initial_itemsetInfo_map, int min_support_count, vector< vector<ItemsetInfo> >& n_item_itemsetInfo_vectors) {

	vector<ItemsetInfo> initial_itemsetInfo_vector;

	for (map<int, ItemsetInfo>::iterator it = initial_itemsetInfo_map.begin(); it != initial_itemsetInfo_map.end(); it++) {
		if (it->second.appearancesInTransactions.size() >= min_support_count)
			initial_itemsetInfo_vector.push_back(it->second);
	}

	// 計算
	n_item_itemsetInfo_vectors = Apriori(min_support_count, initial_itemsetInfo_vector);
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
	// double tStart = omp_get_wtime();

	map<int, ItemsetInfo> initial_itemsetInfo_map;
	// 讀取
	int n_transactions = Read_Input(argv[2], initial_itemsetInfo_map);


	float min_support = stof(argv[1]);
	int min_support_count = 0.5 + min_support * n_transactions;

	// 計算
	vector< vector<ItemsetInfo> > n_item_itemsetInfo_vectors;
	ParaMain(initial_itemsetInfo_map, min_support_count, n_item_itemsetInfo_vectors);

	// 輸出
	Output_Frequent_Pattern(n_item_itemsetInfo_vectors, n_transactions, argv[3]);


	// cout << "Time:" << omp_get_wtime() - tStart << endl;
	return 0;
}