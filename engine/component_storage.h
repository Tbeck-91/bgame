#pragma once

#include <memory>
#include <tuple>
#include <vector>
#include <functional>
#include <utility>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include "mpl_foreach.h"
#include "mpl_typelist.h"
#include "entity_storage.h"

using std::tuple;
using std::vector;
using std::function;
using std::fstream;

namespace engine {
namespace ecs_detail {

template<typename component_list>
class component_storage {
private:
     int next_handle = 1;

     static constexpr size_t number_of_component_types() {
          return component_list::type_list::size();
     }
     //tuple<vector<Ts>...> component_container;
     template<typename ... Ts>
    using tuple_of_vectors = std::tuple<std::vector<Ts>...>;
    ecs_detail::rename<tuple_of_vectors, typename component_list::type_list> component_container;

     template <typename T>
     vector<T> * find_appropriate_bag() {
          return &std::get< vector<T> > ( component_container );
     }

public:
     std::size_t size() {
          std::size_t size = 0;
          for_each ( component_container, [&size] ( auto &x ) {
               size += x.size();
          } );
          return size;
     }
     
     void clear_deleted( entity_storage &entities ) {
          for_each ( component_container, [&entities] ( auto &x ) {
	       if (!x.empty()) {
		  auto new_end = std::remove_if(x.begin(), x.end(), [&entities] (auto &n) {
		    if (n.deleted) {
		      entity * e = entities.find_by_handle( n.entity_id );
		      --e->component_count;
		    }
		    return n.deleted;		    
		  });
		  x.erase(new_end, x.end());
	       }
          } );
     }
     
     void clear_all() {
	  for_each( component_container, [] ( auto &x) { x.clear(); } );
     }

     int get_next_handle() {
          const int result = next_handle;
          ++next_handle;
          return result;
     }

     template <typename T>
     int store_component ( T &component ) {
	  component.handle = get_next_handle();
          find_appropriate_bag<T>()->push_back ( component );
	  return component.handle;
     }

     template <typename T>
     T * find_component_by_handle ( const int &handle_to_find, T ignore ) {
          vector<T> * storage_bag = find_appropriate_bag<T>();
          for ( T &component : *storage_bag ) {
               if ( component.handle == handle_to_find ) {
                    return &component;
               }
          }
          return nullptr;
     }

     template<typename T>
     vector<T *> find_components_by_func ( function<bool ( const T * ) > matcher ) {
          vector<T *> result;
          vector<T> * storage_bag = find_appropriate_bag<T>();
          for ( const T &component : *storage_bag ) {
               if ( matcher ( &component ) ) result.push_back ( &component );
          }
          return result;
     }

     template<typename T>
     vector<T> * find_components_by_type(T ignore) {
          vector<T> * storage_bag = find_appropriate_bag<T>();
          return storage_bag;
     }
     
     template<typename T>
     T * find_entity_component(const int &entity_handle, T ignore) {
	  vector<T> * storage_bag = find_appropriate_bag<T>();
	  for (T &c : *storage_bag) {
	      if (c.entity_id == entity_handle) return &c;
	  }
	  return nullptr;
     }

     void save ( fstream &lbfile ) {
          for_each ( component_container, [&lbfile] ( auto x ) {
               for ( auto &c : x ) {
                    c.save ( lbfile );
               }
          } );
     }
};

}
}
