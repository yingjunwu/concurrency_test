//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// cuckoo_map.cpp
//
// Identification: src/container/cuckoo_map.cpp
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <functional>
#include <iostream>

#include "cuckoo_map.h"

#include <cds/init.h>



CUCKOO_MAP_TEMPLATE_ARGUMENTS
CUCKOO_MAP_TYPE::CuckooMap(){
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
CUCKOO_MAP_TYPE::~CuckooMap(){
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
bool CUCKOO_MAP_TYPE::Insert(const KeyType &key, ValueType value){
  auto status = cuckoo_map.insert(key, value);
  return status;
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
std::pair<bool, bool> CUCKOO_MAP_TYPE::Update(const KeyType &key,
                                              ValueType value,
                                              bool bInsert){

  auto status =  cuckoo_map.update(key, [&value]( bool bNew, map_pair& found_pair) {
    if(bNew == true) {
    }
    else {
      // found existing entry
      found_pair.second = value;
    }
  }, bInsert);

  return status;
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
bool CUCKOO_MAP_TYPE::Erase(const KeyType &key){

  auto status = cuckoo_map.erase(key);

  return status;
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
bool CUCKOO_MAP_TYPE::Find(const KeyType &key,
                           ValueType& value){

  auto status = cuckoo_map.find(key, [&value]( map_pair& found_value) {
    value = found_value.second;
  } );
  return status;
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
bool CUCKOO_MAP_TYPE::Contains(const KeyType &key){
  return cuckoo_map.contains(key);
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
void CUCKOO_MAP_TYPE::Clear(){
  cuckoo_map.clear();
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
size_t CUCKOO_MAP_TYPE::GetSize() const{
  return cuckoo_map.size();
}

CUCKOO_MAP_TEMPLATE_ARGUMENTS
bool CUCKOO_MAP_TYPE::IsEmpty() const{
  return cuckoo_map.empty();
}

// Explicit template instantiation
template class CuckooMap<int64_t, int64_t>;
