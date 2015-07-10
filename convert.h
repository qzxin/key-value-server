#ifndef CONVERT_H
#define CONVERT_H

#include <vector>
#include <iostream>
#include "node.h"

// StrToNode: 传输字符串转换成节点
// Input: s的形式： key:value
//		  s必须包含"：" s不能为NULL
// Output: struct Node*
Node* StrToNode(const std::string &str);

// NodeToStr: 节点转换成传输字符串
// Input: struct Node*  不能为NULL
// Output: s的形式： key:value
std::string NodeToStr(const Node* node);

std::vector<std::string> split_str(const std::string& src, const std::string& delim);

#endif