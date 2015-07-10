/*
 *  hash_cahce.h
 *  
 *  Author: quinn
 *  Date : 2015/06/30
*/
#ifndef HASH_CACHE_H
#define HASH_CACHE_H
#include <vector>
#include <unordered_map>
#include <fstream>
#include <string>
//#include "node.h"
#define PAGE_SIZE 2
#define CACHE_SIZE 2
#define DISK_SIZE 20
#define FAILED -1
using namespace std;
// 类 HashCache 实现了一种基于文件页的LRU Cache的功能，Cache 空间一定，当 Cache 已经被占满时，需要删除最久未被访问过的页。
// 如果该页被修改过，则需要重新写回到文件；
//包含 put 和 get 两种操作
// put 操作：向 缓存中添加该节点的信息，若已存在相同key，则覆盖。删除页时
// get 操作：搜索
class HashCache {
private:
    class Page;
    class Node;
    Page* entries_;
    vector<Page*> free_entries_;  // 存储空闲page
    unordered_map<string, Node* > hash_map; // 存储节点的散列表，以供快速查询
    Page* get_new_page(); // 得到新的一页，可处理没有空页的情况
    Page* put_page;  // 用以标记写入的page
    bool put_page_existed;  // 用于put_page被顶出之后的判断
    const char**file_list_; // 已弃用。文件列表，后来采用直接用文件序号直接构造文件名。
    int new_file_index_; // 新建文件序号，初始为 -1
    Page *head_, *tail_; // 双向列表的头尾，以实现LRU Cache
    void detach(Page* page);  // 卸载该页
    void attach(Page* page);  // 将该页链接到头部

    Node* search(string key);  // 搜索key，成功返回 key 代表的节点，失败返回NULL
    int load_file_to_page(const int &file_num, Page* &page, unordered_map<string, Node* > &hash_map); // 将第file_num个文件加载到page中，并推入hash_map的表中
    int save_page_to_file(Page* &page);  // 将page保存到文件
    void erase_page_of_cache(Page* &page, unordered_map<string, Node* > &hash_map);  // 将该页的内容从hash_map中释放

    int load_new_file_index(const char* save_file_index = "file_index.dat"); // 在该文件中存储着存储文件已经使用到第几个；加载失败返回-1
    void save_new_file_index(const char* save_file_index = "file_index.dat");  // 程序结束时，保存现在的文件序号到文件

public:
    HashCache(int capacity = CACHE_SIZE);
    ~HashCache();
    void save_cache();  //将cache中所有dirty（已修改）内容保存到文件（可以定期调用或者最后结束程序时调用）
    string get(string key);
    void put(string key, string value);
};
#endif
