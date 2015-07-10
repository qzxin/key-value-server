/*
 *  convert.cpp
 *  
 *  Author: quinn
 *  Date : 2015/06/15
*/
#include "convert.h"
using namespace std;
// 传输字符串转换成节点
Node* StrToNode(const string &str) {
    vector<string> substring = split_str(str, "-;:\n");

    Node* node = new Node;
    if (substring.size() == 0) {
        return NULL;
    }
    else if (substring.size() == 1) {
        node->key = substring[0];
        node->value = "failed";
    } else {
        node->key = substring[0];
        node->value = substring[1];
    }
    return node;
}
string NodeToStr(const Node* node) {
    string str;
    if (node == NULL)
        return str;
    str += node->key;
    str += "-";
    str += node->value;
    return str;
}

vector<string> split_str(const string& src, const string& delim) {
    vector<string> dst;
    string str = src;
    str += delim[0]; // 扩展字符串，方便分割最后一节
    string ::size_type start = 0, index;
    index = str.find_first_of(delim, start); //在str中查找(起始：start) delim的任意字符的第一次出现的位置  
    while (index != string::npos) {
        dst.push_back(str.substr(start, index-start)); //substr(start, len)
        start = str.find_first_not_of(delim, index);
        if (start == string::npos)
            return dst;
        index = str.find_first_of(delim, start);
    }
    return dst;
}