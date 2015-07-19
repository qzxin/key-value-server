/*
 *  hash_cahce.cpp
 *  
 *  Author: quinn
 *  Date : 2015/06/30
*/
#include "hash_cache.h"
#include <iostream>
#include <algorithm>
using namespace std;
bool file_accessed[DISK_SIZE] = { false };

// 单个数据节点
class HashCache::Node {
public:
    std::string key_;
    std::string value_;
    class Page* page_;  // 该数据节点所属页
};

// 一页
class HashCache::Page {
public:
    int file_num_; // 页所对应的文件序号
    bool lock_;
    bool dirty_;  // 标记page是否被修改
    bool is_putpape;
    class Node data_[PAGE_SIZE];  // 页包含的节点数据
    class Page* next;
    class Page* prev;
    Page() {
        lock_ = false;
        dirty_ = false;
        is_putpape = false;
    }
};

HashCache::HashCache(int capacity) {
    // file_list_ = file_list;
    new_file_index_ = load_new_file_index(); // the next new file num.
    load_key_file_map(); // load the map of key and its file from keyfile.
    put_page = NULL;
    put_page_existed = false;
    // 为cache分配内存
    entries_ = new Page[capacity];
    for (int i = 0; i < capacity; i++) {
        for (int j = 0; j < PAGE_SIZE; j++)  // 将该页的地址标记到节点中
            (entries_ + i)->data_[j].page_ = entries_ + i;
        free_entries_.push_back(entries_ + i); // 页入队列
    }
    // 构建双向链表
    head_ = new Page;
    tail_ = new Page;
    head_->prev = NULL;
    head_->next = tail_;
    tail_->prev = head_;
    tail_->next = NULL;
}
HashCache::~HashCache() {
    // 保存cache的数据
    save_cache();
    //delete entries_;
    delete head_;
    delete tail_;
}

// 查找 key ： 先在当前 hash_map 中查找;不成功则查找key_file_map,索引现有文件
// 查找文件过程中，如果查找成功，则该文件中的数据保留到hash_map中，所属页链接到LRU双向链表头部；失败，则删除其对应的所有信息，恢复之前的状态
// get 操作 和 put 操作 都要调用此函数
HashCache::Node* HashCache::search(string key) {
    Node* node = hash_map[key];
    Page* page = NULL;
    if (node) {
        // search success, 将节点所属页链到头部
        page = node->page_;
        detach(page);
        attach(page);
    }
    else {
    	int file_num = key_file_map[key]; // 查找(key,filenum)映射表，如果表中不存在该 key，返回 0，否则返回该key对应的文件标号
    	if (file_num == 0)
    		return node;
        page = get_new_page(); // 从空间中获取一个新page
        if (load_file_to_page(file_num, page, hash_map) == FAILED) {
            free_entries_.push_back(page); // 加载失败，重新将该page入栈
        }
        if (node = hash_map[key]) {
            attach(page);
        }
    }
    return node;
}

string HashCache::get(string key) {
    Node* node = search(key);
    if (node)
        return node->value_;
    return string(); // 查找失败返回默认值
}
// put 操作：将数据写入到server；当已经存在同样的key时，覆盖 ;
//        因此，首先先查找是否存在key，存在则覆盖值，并且将页标为dirty；不存在，将数据写入到 put_page 中。
//                  最后都要将操作页链到头部.
//  注：此处的put_page一直接受put操作的值，直至满页后，put_page才指向新的一页，当被弹出时，写入到磁盘
void HashCache::put(string key, string value) {
    Node* node = NULL;
    node = search(key);
    if (node) {  // 存在当前key
        if (value != node->value_) {
            node->value_ = value;
            node->page_->dirty_ = true;
        }
        return;
    }
    else {
        // server 不存在当前节点，写入put_page
        static int data_index = 0;
        // 上一次的 put_page 写满 或 被弹出了，即 put_page 不存在，新的一页新的数据索引
        if (data_index == PAGE_SIZE || put_page_existed == false) {
        	push_key_to_keymap(put_page); // put_page 已满时，将put_page中的key，压入映射表
            data_index = 0;
        }
        if (data_index == 0 || put_page_existed == false) { //和上一语句代码冗余， put_page_existed == false 可以不存在，仅便于理解
                                                            // put_page 初始化
            put_page = get_new_page();
            put_page_existed = true;
            put_page->file_num_ = ++new_file_index_; // put_page 对应文件序号
            put_page->dirty_ = true;
            // initial node
            for (int i = 0; i < PAGE_SIZE; i++) {
                put_page->data_[i].key_ = "";
                put_page->data_[i].value_ = "";
            }
            attach(put_page);
        }
        put_page->data_[data_index].key_ = key;
        put_page->data_[data_index].value_ = value;
        hash_map[key] = put_page->data_ + data_index;  // 加入到散列表
        data_index++;
        detach(put_page);
        attach(put_page);
    }
}


// 将第file_num个文件加载到page中，并推入hash_map的表中
int HashCache::load_file_to_page(const int &file_num, Page* &page, unordered_map<string, Node* > &hash_map) {
    ifstream input;
    string file_name = to_string(file_num) + ".dat"; // 用文件编号构造文件名
    input.open(file_name, ios::in);
    if (!input || page == NULL)
        return FAILED; // 不存在文件
    file_accessed[file_num] = true;  // 表示该文件已经存在于hash_map
    page->file_num_ = file_num;
    Node* data = page->data_;
    for (int i = 0; i < PAGE_SIZE; i++) {
        if (!(input >> data[i].key_ >> data[i].value_))
            data[i].key_ = "", data[i].value_ = ""; // 输入为空
        hash_map[data[i].key_] = data + i;
    }
    input.close();
    return 0;
}
int HashCache::save_page_to_file(Page* &page) {
    page->dirty_ = false;
    if (page == NULL)
        return FAILED;
    ofstream output;
    string file_name = to_string(page->file_num_) + ".dat"; // 用文件编号构造文件名
    output.open(file_name, ios::out);
    Node* data = page->data_;
    for (int i = 0; i < PAGE_SIZE; i++) {
        output << data[i].key_ << " " << data[i].value_ << endl; // 将该页中的节点保存到文件中
    }
    output.close();
    return 0;
}

//加载文件序号，以便server重启后的文件不会产生覆盖；
//加载失败返回-1 （使用前先自增操作，因此从 0 号文件开始）
int HashCache::load_new_file_index(const char* save_file_index) {
    ifstream input;
    input.open(save_file_index, ios::in);
    if (!input) {
        return 0;// 不存在该文件，返回初始序号0
    }
    int index;
    input >> index;
    input.close();
    return index;
}
void HashCache::save_new_file_index(const char* save_file_index) {
    ofstream output;
    output.open(save_file_index, ios::out);
    if (!(output << new_file_index_)) {
        cout << "保存存储文件序号文件失败：write <" << save_file_index << "> failed." << endl;
        exit(-1);
    }
    output.close();
}

void HashCache::erase_page_of_cache(Page* &page, unordered_map<string, Node* > &hash_map) {
    file_accessed[page->file_num_] = false; // 表示该文件已经不再hash_map中
    for (int i = 0; i < PAGE_SIZE; i++) {
        hash_map.erase(page->data_[i].key_);
    }
}

void HashCache::save_cache() {
    // 保存搜索用hash_map
    Page* page = head_->next;
    while (page != tail_) {
        if (page->dirty_ == true) {
            save_page_to_file(page);
        }
        page = page->next;
    }
    save_new_file_index();
    push_key_to_keymap(put_page);
}

void HashCache::detach(Page* page) {
    page->prev->next = page->next;
    page->next->prev = page->prev;
}
void HashCache::attach(Page* page) {
    page->prev = head_;
    page->next = head_->next;
    head_->next->prev = page;
    head_->next = page;
}

// 从cache中分配新的一页：cache有空闲和cache已满
// cache有空闲：从空闲列表中取出一页，返回
// cache已满：需要释放一页才能给出一页空间；
//                      选择释放最后的一页，并且当该页是dirty的，则写入page到文件；当该页是一个未满的put_page，标记为cache中已不存在put_page
// <注> 释放包括：从链表中取出 和 从 hash_map 表中删除该页包含的所有节点信息
HashCache::Page* HashCache::get_new_page() {
    Page* page;
    if (free_entries_.empty() == true) {
        // cache 已满，从尾部取下一个page
        page = tail_->prev;
        if (page == put_page) {
            put_page_existed = false;
        	push_key_to_keymap(page); // 该页将被弹出，保存key，file映射
        } // put_page 被顶出
        if (page->dirty_ == true)
            save_page_to_file(page); // 保存该页，并从hash_map中删除该页
        erase_page_of_cache(page, hash_map);
        detach(page);
        free_entries_.push_back(page); //之所以入栈又出栈，是为了应对读取失败的情况
    }
    page = free_entries_.back();
    free_entries_.pop_back();
    return page;
}
void HashCache::push_key_to_keymap(Page* &page, const char* key_filename) {
	if (page == NULL)
		return;
	ofstream output;
		// save to file (append)
	output.open(key_filename, ios::app);

	Node* data = page->data_;
	int file_num = page->file_num_;
	for (int i = 0; i < PAGE_SIZE; i++) {
		key_file_map[data[i].key_] = file_num;
		output << data[i].key_ << " " << file_num << endl;
	}
	output.close();
}
void HashCache::load_key_file_map(const char* key_filename) {
	ifstream input;
	input.open(key_filename, ios::in);
	if (!input)
		return;
	string key;
	int file_num;
	while (input >> key >> file_num) {
		key_file_map[key] = file_num;
	}
	input.close();
}