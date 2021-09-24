#pragma once
#include <appbase/application.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/fixed_bytes.hpp>
#include <eosio/chain/backing_store/kv_context.hpp>
#include <eosio/chain/backing_store/db_combined.hpp>
#include <eosio/chain/combined_database.hpp>
#include <b1/session/shared_bytes.hpp>
#include <eosio/to_key.hpp>

#include <boost/container/flat_set.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <eosio/chain_plugin/account_query_db.hpp>

#include <fc/static_variant.hpp>
#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp>

namespace fc { class variant; }

namespace eosio {
   using chain::controller;
   using std::unique_ptr;
   using std::pair;
   using namespace appbase;
   using chain::name;
   using chain::uint128_t;
   using chain::public_key_type;
   using chain::transaction;
   using chain::transaction_id_type;
   using boost::container::flat_set;
   using chain::asset;
   using chain::symbol;
   using chain::authority;
   using chain::account_name;
   using chain::action_name;
   using chain::abi_def;
   using chain::abi_serializer;

namespace chain_apis {
struct empty{};

struct permission {
   name              perm_name;
   name              parent;
   authority         required_auth;
};

// see specializations for uint64_t and double in source file
template<typename Type>
Type convert_to_type(const string& str, const string& desc) {
   try {
      return fc::variant(str).as<Type>();
   } FC_RETHROW_EXCEPTIONS(warn, "Could not convert ${desc} string '${str}' to key type.", ("desc", desc)("str",str) )
}

uint64_t convert_to_type(const eosio::name &n, const string &desc);

template<>
uint64_t convert_to_type(const string& str, const string& desc);

template<>
double convert_to_type(const string& str, const string& desc);

template<typename Type>
string convert_to_string(const Type& source, const string& key_type, const string& encode_type, const string& desc);

template<>
string convert_to_string(const chain::key256_t& source, const string& key_type, const string& encode_type, const string& desc);

template<>
string convert_to_string(const float128_t& source, const string& key_type, const string& encode_type, const string& desc);


class keep_processing {
public:
   explicit keep_processing(fc::microseconds&& duration = fc::milliseconds(10)) : end_time_(fc::time_point::now() + duration) {}

   fc::microseconds time_remaining() const { return end_time_ - fc::time_point::now(); }
   bool operator()() const {
      return time_remaining().count() >= 0;
   }
private:
   fc::time_point end_time_;
};

class read_only {
   const controller& db;
   const std::optional<account_query_db>& aqdb;
   const fc::microseconds abi_serializer_max_time;
   bool  shorten_abi_errors = true;

public:
   static const string KEYi64;

   read_only(const controller& db, const std::optional<account_query_db>& aqdb, const fc::microseconds& abi_serializer_max_time)
      : db(db), aqdb(aqdb), abi_serializer_max_time(abi_serializer_max_time) {}
   
   void validate() const {}

   void set_shorten_abi_errors( bool f ) { shorten_abi_errors = f; }

   using get_info_params = empty;

   struct get_info_results {
      string                               server_version;
      chain::chain_id_type                 chain_id;
      uint32_t                             head_block_num = 0;
      uint32_t                             last_irreversible_block_num = 0;
      chain::block_id_type                 last_irreversible_block_id;
      chain::block_id_type                 head_block_id;
      fc::time_point                       head_block_time;
      account_name                         head_block_producer;

      uint64_t                             virtual_block_cpu_limit = 0;
      uint64_t                             virtual_block_net_limit = 0;

      uint64_t                             block_cpu_limit = 0;
      uint64_t                             block_net_limit = 0;
      //string                               recent_slots;
      //double                               participation_rate = 0;
      std::optional<string>                server_version_string;
      std::optional<uint32_t>              fork_db_head_block_num;
      std::optional<chain::block_id_type>  fork_db_head_block_id;
      std::optional<string>                server_full_version_string;
      std::optional<fc::time_point>        last_irreversible_block_time;
      std::optional<uint32_t>              first_block_num;
   };
   get_info_results get_info(const get_info_params&) const;

   struct get_activated_protocol_features_params {
      std::optional<uint32_t>  lower_bound;
      std::optional<uint32_t>  upper_bound;
      uint32_t                 limit = 10;
      bool                     search_by_block_num = false;
      bool                     reverse = false;
   };

   struct get_activated_protocol_features_results {
      fc::variants             activated_protocol_features;
      std::optional<uint32_t>  more;
   };

   get_activated_protocol_features_results get_activated_protocol_features( const get_activated_protocol_features_params& params )const;

   struct producer_info {
      name                       producer_name;
   };

   // account_resource_info holds similar data members as in account_resource_limit, but decoupling making them independently to be refactored in future
   struct account_resource_info {
      int64_t used = 0;
      int64_t available = 0;
      int64_t max = 0;
      std::optional<chain::block_timestamp_type> last_usage_update_time;    // optional for backward nodeos support
      std::optional<int64_t> current_used;  // optional for backward nodeos support
      void set( const chain::resource_limits::account_resource_limit& arl)
      {
         used = arl.used;
         available = arl.available;
         max = arl.max;
         last_usage_update_time = arl.last_usage_update_time;
         current_used = arl.current_used;
      }
   };

   struct get_account_results {
      name                       account_name;
      uint32_t                   head_block_num = 0;
      fc::time_point             head_block_time;

      bool                       privileged = false;
      fc::time_point             last_code_update;
      fc::time_point             created;

      std::optional<asset>       core_liquid_balance;

      int64_t                    ram_quota  = 0;
      int64_t                    net_weight = 0;
      int64_t                    cpu_weight = 0;

      account_resource_info      net_limit;
      account_resource_info      cpu_limit;
      int64_t                    ram_usage = 0;

      vector<permission>         permissions;

      fc::variant                total_resources;
      fc::variant                self_delegated_bandwidth;
      fc::variant                refund_request;
      fc::variant                voter_info;
      fc::variant                rex_info;
   };

   struct get_account_params {
      name                  account_name;
      std::optional<symbol> expected_core_symbol;
   };
   get_account_results get_account( const get_account_params& params )const;


   struct get_code_results {
      name                   account_name;
      string                 wast;
      string                 wasm;
      fc::sha256             code_hash;
      std::optional<abi_def> abi;
   };

   struct get_code_params {
      name account_name;
      bool code_as_wasm = true;
   };

   struct get_code_hash_results {
      name                   account_name;
      fc::sha256             code_hash;
   };

   struct get_code_hash_params {
      name account_name;
   };

   struct get_abi_results {
      name                   account_name;
      std::optional<abi_def> abi;
   };

   struct get_abi_params {
      name account_name;
   };

   struct get_raw_code_and_abi_results {
      name                   account_name;
      chain::blob            wasm;
      chain::blob            abi;
   };

   struct get_raw_code_and_abi_params {
      name                   account_name;
   };

   struct get_raw_abi_params {
      name                      account_name;
      std::optional<fc::sha256> abi_hash;
   };

   struct get_raw_abi_results {
      name                       account_name;
      fc::sha256                 code_hash;
      fc::sha256                 abi_hash;
      std::optional<chain::blob> abi;
   };


   get_code_results get_code( const get_code_params& params )const;
   get_code_hash_results get_code_hash( const get_code_hash_params& params )const;
   get_abi_results get_abi( const get_abi_params& params )const;
   get_raw_code_and_abi_results get_raw_code_and_abi( const get_raw_code_and_abi_params& params)const;
   get_raw_abi_results get_raw_abi( const get_raw_abi_params& params)const;



   struct abi_json_to_bin_params {
      name         code;
      name         action;
      fc::variant  args;
   };
   struct abi_json_to_bin_result {
      vector<char>   binargs;
   };

   abi_json_to_bin_result abi_json_to_bin( const abi_json_to_bin_params& params )const;


   struct abi_bin_to_json_params {
      name         code;
      name         action;
      vector<char> binargs;
   };
   struct abi_bin_to_json_result {
      fc::variant    args;
   };

   abi_bin_to_json_result abi_bin_to_json( const abi_bin_to_json_params& params )const;


   struct get_required_keys_params {
      fc::variant transaction;
      flat_set<public_key_type> available_keys;
   };
   struct get_required_keys_result {
      flat_set<public_key_type> required_keys;
   };

   get_required_keys_result get_required_keys( const get_required_keys_params& params)const;

   using get_transaction_id_params = transaction;
   using get_transaction_id_result = transaction_id_type;

   get_transaction_id_result get_transaction_id( const get_transaction_id_params& params)const;

   struct get_block_params {
      string block_num_or_id;
   };

   fc::variant get_block(const get_block_params& params) const;

   struct get_block_info_params {
      uint32_t block_num;
   };

   fc::variant get_block_info(const get_block_info_params& params) const;

   struct get_block_header_state_params {
      string block_num_or_id;
   };

   fc::variant get_block_header_state(const get_block_header_state_params& params) const;

   struct get_table_rows_params {
      bool                 json = false;
      name                 code;
      string               scope;
      name                 table;
      string               table_key;
      string               lower_bound;
      string               upper_bound;
      uint32_t             limit = 10;
      string               key_type;  // type of key specified by index_position
      string               index_position; // 1 - primary (first), 2 - secondary index (in order defined by multi_index), 3 - third index, etc
      string               encode_type{"dec"}; //dec, hex , default=dec
      std::optional<bool>  reverse;
      std::optional<bool>  show_payer; // show RAM pyer
    };

   struct get_kv_table_rows_params {
        bool                   json = false;          // true if you want output rows in json format, false as variant
        name                   code;                  // name of contract
        name                   table;                 // name of kv table,
        name                   index_name;            // name of index index
        string                 encode_type;           // encoded type for values in index_value/lower_bound/upper_bound
        string                 index_value;           // index value for point query.  If this is set, it is processed as a point query
        string                 lower_bound;           // lower bound value of index of index_name. If index_value is not set and lower_bound is not set, return from the beginning of range in the prefix
        string                 upper_bound;           // upper bound value of index of index_name, If index_value is not set and upper_bound is not set, It is set to the beginning of the next prefix range.
        uint32_t               limit = 10;            // max number of rows
        bool                   reverse = false;       // if true output rows in reverse order
        bool                   show_payer = false;
   };

   struct get_table_rows_result {
      vector<fc::variant> rows; ///< one row per item, either encoded as hex String or JSON object
      bool                more = false; ///< true if last element in data is not the end and sizeof data() < limit
      string              next_key; ///< fill lower_bound with this value to fetch more rows
      string              next_key_bytes; ///< fill lower_bound with this value to fetch more rows with encode-type of "bytes"
   };

   get_table_rows_result get_table_rows( const get_table_rows_params& params )const;

   get_table_rows_result get_kv_table_rows( const get_kv_table_rows_params& params )const;

   struct get_table_by_scope_params {
      name                 code; // mandatory
      name                 table; // optional, act as filter
      string               lower_bound; // lower bound of scope, optional
      string               upper_bound; // upper bound of scope, optional
      uint32_t             limit = 10;
      std::optional<bool>  reverse;
   };
   struct get_table_by_scope_result_row {
      name        code;
      name        scope;
      name        table;
      name        payer;
      uint32_t    count;
   };
   struct get_table_by_scope_result {
      vector<get_table_by_scope_result_row> rows;
      string      more; ///< fill lower_bound with this value to fetch more rows
   };

   get_table_by_scope_result get_table_by_scope( const get_table_by_scope_params& params )const;

   struct get_currency_balance_params {
      name                  code;
      name                  account;
      std::optional<string> symbol;
   };

   vector<asset> get_currency_balance( const get_currency_balance_params& params )const;

   struct get_currency_stats_params {
      name           code;
      string         symbol;
   };


   struct get_currency_stats_result {
      asset          supply;
      asset          max_supply;
      account_name   issuer;
   };

   fc::variant get_currency_stats( const get_currency_stats_params& params )const;

   struct get_producers_params {
      bool        json = false;
      string      lower_bound;
      uint32_t    limit = 50;
   };

   struct get_producers_result {
      vector<fc::variant> rows; ///< one row per item, either encoded as hex string or JSON object
      double              total_producer_vote_weight;
      string              more; ///< fill lower_bound with this value to fetch more rows
   };

   get_producers_result get_producers( const get_producers_params& params )const;

   struct get_producer_schedule_params {
   };

   struct get_producer_schedule_result {
      fc::variant active;
      fc::variant pending;
      fc::variant proposed;
   };

   get_producer_schedule_result get_producer_schedule( const get_producer_schedule_params& params )const;

   struct get_scheduled_transactions_params {
      bool        json = false;
      string      lower_bound;  /// timestamp OR transaction ID
      uint32_t    limit = 50;
   };

   struct get_scheduled_transactions_result {
      fc::variants  transactions;
      string        more; ///< fill lower_bound with this to fetch next set of transactions
   };

   get_scheduled_transactions_result get_scheduled_transactions( const get_scheduled_transactions_params& params ) const;

   eosio::chain::backing_store_type get_backing_store() const;

   enum class row_requirements { required, optional };
   template<typename Function>
   bool get_primary_key_internal(name code, name scope, name table, uint64_t primary_key, row_requirements require_table,
                                 row_requirements require_primary, Function&& f) const {
      const auto db_backing_store = get_backing_store();
      if (db_backing_store == eosio::chain::backing_store_type::CHAINBASE) {
         const auto* const table_id =
               db.db().find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(code, scope, table));
         if (require_table == row_requirements::optional && !table_id) {
            return false;
         }
         EOS_ASSERT(table_id, chain::contract_table_query_exception,
                    "Missing code: ${code}, scope: ${scope}, table: ${table}",
                    ("code",code.to_string())("scope",scope.to_string())("table",table.to_string()));
         const auto& kv_index = db.db().get_index<chain::key_value_index, chain::by_scope_primary>();
         const auto it = kv_index.find(boost::make_tuple(table_id->id, primary_key));
         if (require_primary == row_requirements::optional && it == kv_index.end()) {
            return false;
         }
         EOS_ASSERT(it != kv_index.end(), chain::contract_table_query_exception,
                    "Missing row for primary_key: ${primary} in code: ${code}, scope: ${scope}, table: ${table}",
                    ("primary", primary_key)("code",code.to_string())("scope",scope.to_string())
                    ("table",table.to_string()));
         f(*it);
         return true;
      }
      else {
         using namespace eosio::chain;
         EOS_ASSERT(db_backing_store == backing_store_type::ROCKSDB,
                    chain::contract_table_query_exception,
                    "Support for configured backing_store has not been added to get_primary_key");
         const auto& kv_database = db.kv_db();
         const auto full_key = chain::backing_store::db_key_value_format::create_full_primary_key(code, scope, table, primary_key);
         auto current_session = kv_database.get_kv_undo_stack()->top();
         const auto value = current_session.read(full_key);
         // check if we didn't actually find the key
         if (!value) {
            // only need to bother to do table search if we require it, so that we can report the correct error
            if (require_table == row_requirements::required) {
               const auto whole_table_prefix(backing_store::db_key_value_format::create_full_key_prefix(full_key, backing_store::db_key_value_format::end_of_prefix::pre_type));
               const auto value = current_session.read(whole_table_prefix);
               EOS_ASSERT(value, chain::contract_table_query_exception,
                          "Missing code: ${code}, scope: ${scope}, table: ${table}",
                          ("code",code.to_string())("scope",scope.to_string())("table",table.to_string()));
            }
            EOS_ASSERT(require_primary == row_requirements::optional, chain::contract_table_query_exception,
                       "Missing row for primary_key: ${primary} in code: ${code}, scope: ${scope}, table: ${table}",
                       ("primary", primary_key)("code",code.to_string())("scope",scope.to_string())
                       ("table",table.to_string()));
            return false;
         }
         f(chain::backing_store::primary_index_view::create(primary_key, value->data(), value->size()));
         return true;
      }
   }

   template<typename T, typename Function>
   bool get_primary_key(name code, name scope, name table, uint64_t primary_key, row_requirements require_table,
                        row_requirements require_primary, Function&& f) const {
      auto ret = get_primary_key_internal(code, scope, table, primary_key, require_table, require_primary, [&f](const auto& obj) {
         if( obj.value.size() >= sizeof(T) ) {
            T t;
            fc::datastream<const char *> ds(obj.value.data(), obj.value.size());
            fc::raw::unpack(ds, t);

            f(t);
         }
      });
      return ret;
   }

   fc::variant get_primary_key(name code, name scope, name table, uint64_t primary_key, row_requirements require_table,
                               row_requirements require_primary, const std::string_view& type, bool as_json = true) const;
   fc::variant get_primary_key(name code, name scope, name table, uint64_t primary_key, row_requirements require_table,
                               row_requirements require_primary, const std::string_view& type, const abi_serializer& abis,
                               bool as_json = true) const;

   auto get_primary_key_value(const std::string_view& type, const abi_serializer& abis, bool as_json = true) const {
      return [table_type=std::string{type},abis,as_json,this](fc::variant& result_var, const auto& obj) {
         vector<char> data;
         read_only::copy_inline_row(obj, data);
         if (as_json) {
            result_var = abis.binary_to_variant(table_type, data, abi_serializer::create_yield_function( abi_serializer_max_time ), shorten_abi_errors );
         }
         else {
            result_var = fc::variant(data);
         }
      };
   }

   auto get_primary_key_value(fc::variant& result_var, const std::string_view& type, const abi_serializer& abis, bool as_json = true) const {
      auto get_primary = get_primary_key_value(type, abis, as_json);
      return [&result_var,get_primary{std::move(get_primary)}](const auto& obj) {
         return get_primary(result_var, obj);
      };
   }

   auto get_primary_key_value(name table, const abi_serializer& abis, bool as_json, const std::optional<bool>& show_payer) const {
      return [abis,table,show_payer,as_json,this](const auto& obj) -> fc::variant {
         fc::variant data_var;
         auto get_prim = get_primary_key_value(data_var, abis.get_table_type(table), abis, as_json);
         get_prim(obj);

         if( show_payer && *show_payer ) {
            return fc::mutable_variant_object("data", std::move(data_var))("payer", obj.payer);
         } else {
            return data_var;
         }
      };
   }

   struct send_ro_transaction_params_v1 {
      bool return_failure_traces = false;
      fc::variant transaction;
   };

   struct send_ro_transaction_results {
      uint32_t                     head_block_num = 0;
      chain::block_id_type         head_block_id;
      uint32_t                     last_irreversible_block_num = 0;
      chain::block_id_type         last_irreversible_block_id;
      digest_type                  code_hash;
      vector<transaction_id_type>  pending_transactions;
      fc::variant                  result;
   };

   void send_ro_transaction(const send_ro_transaction_params_v1& params, chain::plugin_interface::next_function<send_ro_transaction_results> next ) const;

   template<typename KeyValueObj>
   static void copy_inline_row(const KeyValueObj& obj, vector<char>& data) {
      data.resize( obj.value.size() );
      memcpy( data.data(), obj.value.data(), obj.value.size() );
   }

   template<typename Func>
   struct primary_key_receiver
   : chain::backing_store::single_type_error_receiver<primary_key_receiver<Func>, chain::backing_store::primary_index_view, chain::contract_table_query_exception> {
      primary_key_receiver(Func f) : f_(f) {}

      void add_only_row(const chain::backing_store::primary_index_view& row) {
         if(!f_(row))
            done_ = true;
      }

      void add_table_row(const chain::backing_store::table_id_object_view& ) {
         // used for only one table, so we already know the context of the table
      }

      auto keep_processing_entries() {
         return [&done=done_]() {
            return !done;
         };
      };

      Func f_;
      bool done_ = false;
   };

   template<typename Function>
   void walk_key_value_table(const name& code, const name& scope, const name& table, Function f) const
   {
      const auto db_backing_store = get_backing_store();
      if (db_backing_store == eosio::chain::backing_store_type::CHAINBASE) {
         const auto& d = db.db();
         const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(code, scope, table));
         if (t_id != nullptr) {
            const auto &idx = d.get_index<chain::key_value_index, chain::by_scope_primary>();
            decltype(t_id->id) next_tid(t_id->id._id + 1);
            auto lower = idx.lower_bound(boost::make_tuple(t_id->id));
            auto upper = idx.lower_bound(boost::make_tuple(next_tid));

            for (auto itr = lower; itr != upper; ++itr) {
               if (!f(*itr)) {
                  break;
               }
            }
         }
      }
      else {
         using namespace eosio::chain;
         EOS_ASSERT(db_backing_store == backing_store_type::ROCKSDB,
                    chain::contract_table_query_exception,
                    "Support for configured backing_store has not been added to get_primary_key");
            primary_key_receiver<Function> receiver(f);
         auto kp = receiver.keep_processing_entries();
         backing_store::rocksdb_contract_db_table_writer<primary_key_receiver<Function>, std::decay_t < decltype(kp)>> writer(receiver, backing_store::key_context::standalone, kp);
         const auto& kv_database = db.kv_db();
         using key_type = chain::backing_store::db_key_value_format::key_type;
         auto start = chain::backing_store::db_key_value_format::create_full_prefix_key(code, scope, table, key_type::primary);
         auto end = start.next();
         eosio::chain::backing_store::walk_rocksdb_entries_with_prefix(kv_database.get_kv_undo_stack(), start, end, writer);
      }
   }

   static uint64_t get_table_index_name(const read_only::get_table_rows_params& p, bool& primary);


   template<typename Index, typename Function>
   struct secondary_key_receiver
   : chain::backing_store::single_type_error_receiver<secondary_key_receiver<Index, Function>, chain::backing_store::secondary_index_view<Index>, chain::contract_table_query_exception> {
      secondary_key_receiver(read_only::get_table_rows_result& result, Function f, const read_only::get_table_rows_params& params)
      : result_(result), f_(f), params_(params) {}

      void add_only_row(const chain::backing_store::secondary_index_view<Index>& row) {
         // needs to allow a second pass after limit is reached or time has passed, to allow "more" processing
         if (reached_limit_ || !kp_()) {
            result_.more = true;
            result_.next_key = convert_to_string(row.secondary_key, params_.key_type, params_.encode_type, "next_key - next lower bound");
            done_ = true;
         }
         else {
            f_(row, result_.rows);
            reached_limit_ |= result_.rows.size() >= params_.limit;
         }
      }

      void add_table_row(const chain::backing_store::table_id_object_view& ) {
         // used for only one table, so we already know the context of the table
      }

      auto keep_processing_entries() {
         return [&done=done_]() {
            return !done;
         };
      };

      read_only::get_table_rows_result& result_;
      Function f_;
      const read_only::get_table_rows_params& params_;
      bool reached_limit_ = false;
      bool done_ = false;
      keep_processing kp_;
   };


   template <typename IndexType, typename SecKeyType, typename ConvFn>
   read_only::get_table_rows_result get_table_rows_by_seckey( const read_only::get_table_rows_params& p, const abi_def& abi, ConvFn conv )const {
      read_only::get_table_rows_result result;
      const auto& d = db.db();

      name scope{ convert_to_type<uint64_t>(p.scope, "scope") };

      abi_serializer abis;
      abis.set_abi(abi, abi_serializer::create_yield_function( abi_serializer_max_time ) );
      bool primary = false;
      const uint64_t table_with_index = get_table_index_name(p, primary);
      using secondary_key_type = std::result_of_t<decltype(conv)(SecKeyType)>;
      static_assert( std::is_same<typename IndexType::value_type::secondary_key_type, secondary_key_type>::value, "Return type of conv does not match type of secondary key for IndexType" );
      auto secondary_key_lower = eosio::chain::secondary_key_traits<secondary_key_type>::true_lowest();
      const auto primary_key_lower = std::numeric_limits<uint64_t>::lowest();
      auto secondary_key_upper = eosio::chain::secondary_key_traits<secondary_key_type>::true_highest();
      const auto primary_key_upper = std::numeric_limits<uint64_t>::max();
      if( p.lower_bound.size() ) {
         if( p.key_type == "name" ) {
            if constexpr (std::is_same_v<uint64_t, SecKeyType>) {
               SecKeyType lv = convert_to_type(name{p.lower_bound}, "lower_bound name");
               secondary_key_lower = conv( lv );
            } else {
               EOS_ASSERT(false, chain::contract_table_query_exception, "Invalid key type of eosio::name ${nm} for lower bound", ("nm", p.lower_bound));
            }
         } else {
            SecKeyType lv = convert_to_type<SecKeyType>( p.lower_bound, "lower_bound" );
            secondary_key_lower = conv( lv );
         }
      }

      if( p.upper_bound.size() ) {
         if( p.key_type == "name" ) {
            if constexpr (std::is_same_v<uint64_t, SecKeyType>) {
               SecKeyType uv = convert_to_type(name{p.upper_bound}, "upper_bound name");
               secondary_key_upper = conv( uv );
            } else {
               EOS_ASSERT(false, chain::contract_table_query_exception, "Invalid key type of eosio::name ${nm} for upper bound", ("nm", p.upper_bound));
            }
         } else {
            SecKeyType uv = convert_to_type<SecKeyType>( p.upper_bound, "upper_bound" );
            secondary_key_upper = conv( uv );
         }
      }
      if( secondary_key_upper < secondary_key_lower )
         return result;

      const bool reverse = p.reverse && *p.reverse;
      const auto db_backing_store = get_backing_store();
      auto get_prim_key_val = get_primary_key_value(p.table, abis, p.json, p.show_payer);
      if (db_backing_store == eosio::chain::backing_store_type::CHAINBASE) {
         const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(p.code, scope, p.table));
         const auto* index_t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(p.code, scope, name(table_with_index)));
         if( t_id != nullptr && index_t_id != nullptr ) {

            const auto& secidx = d.get_index<IndexType, chain::by_secondary>();
            auto lower_bound_lookup_tuple = std::make_tuple( index_t_id->id._id,
                                                            secondary_key_lower,
                                                            primary_key_lower );
            auto upper_bound_lookup_tuple = std::make_tuple( index_t_id->id._id,
                                                            secondary_key_upper,
                                                            primary_key_upper );

            auto walk_table_row_range = [&]( auto itr, auto end_itr ) {
               keep_processing kp;
               vector<char> data;
               for( unsigned int count = 0; kp() && count < p.limit && itr != end_itr; ++itr ) {
                  const auto* itr2 = d.find<chain::key_value_object, chain::by_scope_primary>( boost::make_tuple(t_id->id, itr->primary_key) );
                  if( itr2 == nullptr ) continue;

                  result.rows.emplace_back( get_prim_key_val(*itr2) );

                  ++count;
               }
               if( itr != end_itr ) {
                  result.more = true;
                  result.next_key = convert_to_string(itr->secondary_key, p.key_type, p.encode_type, "next_key - next lower bound");
               }
            };

            auto lower = secidx.lower_bound( lower_bound_lookup_tuple );
            auto upper = secidx.upper_bound( upper_bound_lookup_tuple );
            if( reverse ) {
               walk_table_row_range( boost::make_reverse_iterator(upper), boost::make_reverse_iterator(lower) );
            } else {
               walk_table_row_range( lower, upper );
            }
         }
      }
      else {
         using namespace eosio::chain;
         EOS_ASSERT(db_backing_store == backing_store_type::ROCKSDB,
                    chain::contract_table_query_exception,
                    "Support for configured backing_store has not been added to get_primary_key");
         const auto context = (reverse) ? backing_store::key_context::standalone_reverse : backing_store::key_context::standalone;
         auto lower = chain::backing_store::db_key_value_format::create_full_prefix_secondary_key(p.code, scope, name(table_with_index), secondary_key_lower);
         auto upper = chain::backing_store::db_key_value_format::create_full_prefix_secondary_key(p.code, scope, name(table_with_index), secondary_key_upper);
         if (reverse) {
            lower = eosio::session::shared_bytes::truncate_key(lower);
         }
         // since upper is either the upper_bound of a forward search, or the reverse iterator <= for the beginning of the end of
         // this secondary type, we need to move it to just before the beginning of the next type
         upper = upper.next();
         const auto& kv_database = db.kv_db();
         auto session = kv_database.get_kv_undo_stack()->top();
         auto get_primary = [code=p.code,scope,table=p.table,&session,&get_prim_key_val](const chain::backing_store::secondary_index_view<secondary_key_type>& row, vector<fc::variant>& rows) {
            auto full_key = chain::backing_store::db_key_value_format::create_full_primary_key(code, scope, table, row.primary_key);
            auto value = session.read(full_key);
            if( !value ) return;

            rows.emplace_back(get_prim_key_val(chain::backing_store::primary_index_view::create(row.primary_key, value->data(), value->size())));
         };
         using secondary_receiver = secondary_key_receiver<secondary_key_type, decltype(get_primary)>;
         secondary_receiver receiver(result, get_primary, p);
         auto kp = receiver.keep_processing_entries();
         backing_store::rocksdb_contract_db_table_writer<secondary_receiver, std::decay_t < decltype(kp)>> writer(receiver, context, kp);
         eosio::chain::backing_store::walk_rocksdb_entries_with_prefix(kv_database.get_kv_undo_stack(), lower, upper, writer);
      }

      return result;
   }

   template <typename IndexType>
   read_only::get_table_rows_result get_table_rows_ex( const read_only::get_table_rows_params& p, const abi_def& abi )const {
      read_only::get_table_rows_result result;
      const auto& d = db.db();

      name scope { convert_to_type<uint64_t>(p.scope, "scope") };

      abi_serializer abis;
      abis.set_abi(abi, abi_serializer::create_yield_function( abi_serializer_max_time ));

      auto primary_lower = std::numeric_limits<uint64_t>::lowest();
      auto primary_upper = std::numeric_limits<uint64_t>::max();

      if( p.lower_bound.size() ) {
         if( p.key_type == "name" ) {
            name s(p.lower_bound);
            primary_lower = s.to_uint64_t();
         } else {
            auto lv = convert_to_type<typename IndexType::value_type::key_type>( p.lower_bound, "lower_bound" );
            primary_lower = lv;
         }
      }

      if( p.upper_bound.size() ) {
         if( p.key_type == "name" ) {
            name s(p.upper_bound);
            primary_upper = s.to_uint64_t();
         } else {
            auto uv = convert_to_type<typename IndexType::value_type::key_type>( p.upper_bound, "upper_bound" );
            primary_upper = uv;
         }
      }

      if( primary_upper < primary_lower )
         return result;

      auto get_prim_key = get_primary_key_value(p.table, abis, p.json, p.show_payer);
      auto handle_more = [&result,&p](const auto& row) {
         result.more = true;
         result.next_key = convert_to_string(row.primary_key, p.key_type, p.encode_type, "next_key - next lower bound");
      };

      const bool reverse = p.reverse && *p.reverse;
      const auto db_backing_store = get_backing_store();
      if (db_backing_store == eosio::chain::backing_store_type::CHAINBASE) {
         const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(p.code, scope, p.table));
         if( t_id != nullptr ) {
            const auto& idx = d.get_index<IndexType, chain::by_scope_primary>();
            auto lower_bound_lookup_tuple = std::make_tuple( t_id->id, primary_lower );
            auto upper_bound_lookup_tuple = std::make_tuple( t_id->id, primary_upper );

            auto walk_table_row_range = [&]( auto itr, auto end_itr ) {
               keep_processing kp;
               vector<char> data;
               for( unsigned int count = 0; kp() && count < p.limit && itr != end_itr; ++count, ++itr ) {
                  result.rows.emplace_back( get_prim_key(*itr) );
               }
               if( itr != end_itr ) {
                  handle_more(*itr);
               }
            };

            auto lower = idx.lower_bound( lower_bound_lookup_tuple );
            auto upper = idx.upper_bound( upper_bound_lookup_tuple );
            if( reverse ) {
               walk_table_row_range( boost::make_reverse_iterator(upper), boost::make_reverse_iterator(lower) );
            } else {
               walk_table_row_range( lower, upper );
            }
         }
      }
      else {
         using namespace eosio::chain;
         EOS_ASSERT(db_backing_store == backing_store_type::ROCKSDB,
                    chain::contract_table_query_exception,
                    "Support for configured backing_store has not been added to get_primary_key");
         const auto context = (reverse) ? backing_store::key_context::standalone_reverse : backing_store::key_context::standalone;
         auto lower = chain::backing_store::db_key_value_format::create_full_primary_key(p.code, scope, p.table, primary_lower);
         auto upper = chain::backing_store::db_key_value_format::create_full_primary_key(p.code, scope, p.table, primary_upper);
         if (reverse) {
            lower = eosio::session::shared_bytes::truncate_key(lower);
         }
         // since upper is either the upper_bound of a forward search, or the reverse iterator <= for the beginning of the end of
         // this secondary type, we need to move it to just before the beginning of the next type
         upper = upper.next();
         const auto& kv_database = db.kv_db();

         keep_processing kp;
         auto filter_primary_key = [&kp,&result,&p,&get_prim_key,&handle_more](const backing_store::primary_index_view& row) {
            if (!kp() || result.rows.size() >= p.limit) {
               handle_more(row);
               return false;
            }
            else {
               result.rows.emplace_back(get_prim_key(row));
               return true;
            }
         };

         using primary_receiver = primary_key_receiver<decltype(filter_primary_key)>;
         primary_receiver receiver(filter_primary_key);
         auto keep_processing_entries = receiver.keep_processing_entries();
         backing_store::rocksdb_contract_db_table_writer<primary_receiver, std::decay_t < decltype(keep_processing_entries)>> writer(receiver, context, keep_processing_entries);
         eosio::chain::backing_store::walk_rocksdb_entries_with_prefix(kv_database.get_kv_undo_stack(), lower, upper, writer);
      }
      return result;
   }

   using get_accounts_by_authorizers_result = account_query_db::get_accounts_by_authorizers_result;
   using get_accounts_by_authorizers_params = account_query_db::get_accounts_by_authorizers_params;
   get_accounts_by_authorizers_result get_accounts_by_authorizers( const get_accounts_by_authorizers_params& args) const;

   struct get_all_accounts_result{
      struct account_result {
         chain::name                          name;
         // chain::account_object::id_type       id;   
         chain::block_timestamp_type          creation_date;
      };

      std::vector<account_result> accounts;
   };

   struct get_all_accounts_params {
      uint16_t page;
      uint16_t page_size;
   };
     
   get_all_accounts_result get_all_accounts( const get_all_accounts_params& params)  const;
   
   chain::symbol extract_core_symbol()const;
};

class read_write {
   controller& db;
   const fc::microseconds abi_serializer_max_time;
   const bool api_accept_transactions;
public:
   read_write(controller& db, const fc::microseconds& abi_serializer_max_time, bool api_accept_transactions);
   void validate() const;

   using push_block_params_v1 = chain::signed_block_v0;
   using push_block_results = empty;
   void push_block(push_block_params_v1&& params, chain::plugin_interface::next_function<push_block_results> next);

   using push_transaction_params_v1 = fc::variant_object;
   struct push_transaction_results {
      chain::transaction_id_type  transaction_id;
      fc::variant                 processed;
   };
   void push_transaction(const push_transaction_params_v1& params, chain::plugin_interface::next_function<push_transaction_results> next);

   using push_transactions_params_v1  = vector<push_transaction_params_v1>;
   using push_transactions_results = vector<push_transaction_results>;
   void push_transactions(const push_transactions_params_v1& params, chain::plugin_interface::next_function<push_transactions_results> next);

   using send_transaction_params_v1 = push_transaction_params_v1;
   using send_transaction_results = push_transaction_results;
   void send_transaction(const send_transaction_params_v1& params, chain::plugin_interface::next_function<send_transaction_results> next);

   struct send_transaction_params_v2 {
      bool return_failure_traces = false;
      fc::variant transaction;
   };
   void send_transaction(const send_transaction_params_v2& params, chain::plugin_interface::next_function<send_transaction_results> next);

   void send_transaction(packed_transaction_ptr input_trx, const std::string method, bool return_failure_traces, chain::plugin_interface::next_function<send_transaction_results> next);

};

 //support for --key_types [sha256,ripemd160] and --encoding [dec/hex]
 constexpr const char i64[]       = "i64";
 constexpr const char i128[]      = "i128";
 constexpr const char i256[]      = "i256";
 constexpr const char float64[]   = "float64";
 constexpr const char float128[]  = "float128";
 constexpr const char sha256[]    = "sha256";
 constexpr const char ripemd160[] = "ripemd160";
 constexpr const char dec[]       = "dec";
 constexpr const char hex[]       = "hex";


 template<const char*key_type , const char *encoding=chain_apis::dec>
 struct keytype_converter ;

 template<>
 struct keytype_converter<chain_apis::sha256, chain_apis::hex> {
     using input_type = chain::checksum256_type;
     using index_type = chain::index256_index;
     static auto function() {
        return [](const input_type& v) {
            // The input is in big endian, i.e. f58262c8005bb64b8f99ec6083faf050c502d099d9929ae37ffed2fe1bb954fb
            // fixed_bytes will convert the input to array of 2 uint128_t in little endian, i.e. 50f0fa8360ec998f4bb65b00c86282f5 fb54b91bfed2fe7fe39a92d999d002c5
            // which is the format used by secondary index
            uint8_t buffer[32];
            memcpy(buffer, v.data(), 32);
            fixed_bytes<32> fb(buffer); 
            return chain::key256_t(fb.get_array());
        };
     }
 };

 //key160 support with padding zeros in the end of key256
 template<>
 struct keytype_converter<chain_apis::ripemd160, chain_apis::hex> {
     using input_type = chain::checksum160_type;
     using index_type = chain::index256_index;
     static auto function() {
        return [](const input_type& v) {
            // The input is in big endian, i.e. 83a83a3876c64c33f66f33c54f1869edef5b5d4a000000000000000000000000
            // fixed_bytes will convert the input to array of 2 uint128_t in little endian, i.e. ed69184fc5336ff6334cc676383aa883 0000000000000000000000004a5d5bef
            // which is the format used by secondary index
            uint8_t buffer[20];
            memcpy(buffer, v.data(), 20);
            fixed_bytes<20> fb(buffer); 
            return chain::key256_t(fb.get_array());
        };
     }
 };

 template<>
 struct keytype_converter<chain_apis::i256> {
     using input_type = boost::multiprecision::uint256_t;
     using index_type = chain::index256_index;
     static auto function() {
        return [](const input_type v) {
            // The input is in little endian of uint256_t, i.e. fb54b91bfed2fe7fe39a92d999d002c550f0fa8360ec998f4bb65b00c86282f5
            // the following will convert the input to array of 2 uint128_t in little endian, i.e. 50f0fa8360ec998f4bb65b00c86282f5 fb54b91bfed2fe7fe39a92d999d002c5
            // which is the format used by secondary index
            chain::key256_t k;
            uint8_t buffer[32];
            boost::multiprecision::export_bits(v, buffer, 8, false);
            memcpy(&k[0], buffer + 16, 16);
            memcpy(&k[1], buffer, 16);
            return k;
        };
     }
 };

} // namespace chain_apis

class chain_plugin : public plugin<chain_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((blockvault_client_plugin))
   
   chain_plugin();
   virtual ~chain_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();
   void handle_sighup() override;

   chain_apis::read_write get_read_write_api() { return chain_apis::read_write(chain(), get_abi_serializer_max_time(), api_accept_transactions()); }
   chain_apis::read_only get_read_only_api() const;
   
   bool accept_block( const chain::signed_block_ptr& block, const chain::block_id_type& id );
   void accept_transaction(const chain::packed_transaction_ptr& trx, chain::plugin_interface::next_function<chain::transaction_trace_ptr> next);

   // Only call this after plugin_initialize()!
   controller& chain();
   // Only call this after plugin_initialize()!
   const controller& chain() const;

   chain::chain_id_type get_chain_id() const;
   fc::microseconds get_abi_serializer_max_time() const;
   bool api_accept_transactions() const;
   // set true by other plugins if any plugin allows transactions
   bool accept_transactions() const;
   void enable_accept_transactions();

   static void handle_guard_exception(const chain::guard_exception& e);
   void do_hard_replay(const variables_map& options);

   static void handle_db_exhaustion();
   static void handle_bad_alloc();
   
   bool account_queries_enabled() const;

   fc::variant get_entire_trx_trace(const transaction_trace_ptr& trx_trace) const;
   fc::variant get_entire_trx(const transaction& trx) const;

private:
   static void log_guard_exception(const chain::guard_exception& e);

   unique_ptr<class chain_plugin_impl> my;
};

}

FC_REFLECT( eosio::chain_apis::permission, (perm_name)(parent)(required_auth) )
FC_REFLECT(eosio::chain_apis::empty, )
FC_REFLECT(eosio::chain_apis::read_only::get_info_results,
           (server_version)(chain_id)(head_block_num)(last_irreversible_block_num)(last_irreversible_block_id)
           (head_block_id)(head_block_time)(head_block_producer)
           (virtual_block_cpu_limit)(virtual_block_net_limit)(block_cpu_limit)(block_net_limit)
           (server_version_string)(fork_db_head_block_num)(fork_db_head_block_id)(server_full_version_string)
           (last_irreversible_block_time) (first_block_num) )
FC_REFLECT(eosio::chain_apis::read_only::get_activated_protocol_features_params, (lower_bound)(upper_bound)(limit)(search_by_block_num)(reverse) )
FC_REFLECT(eosio::chain_apis::read_only::get_activated_protocol_features_results, (activated_protocol_features)(more) )
FC_REFLECT(eosio::chain_apis::read_only::get_block_params, (block_num_or_id))
FC_REFLECT(eosio::chain_apis::read_only::get_block_info_params, (block_num))
FC_REFLECT(eosio::chain_apis::read_only::get_block_header_state_params, (block_num_or_id))

FC_REFLECT( eosio::chain_apis::read_write::send_transaction_params_v2, (return_failure_traces)(transaction) )
FC_REFLECT( eosio::chain_apis::read_write::push_transaction_results, (transaction_id)(processed) )

FC_REFLECT( eosio::chain_apis::read_only::get_table_rows_params, (json)(code)(scope)(table)(table_key)(lower_bound)(upper_bound)(limit)(key_type)(index_position)(encode_type)(reverse)(show_payer) )
FC_REFLECT( eosio::chain_apis::read_only::get_kv_table_rows_params, (json)(code)(table)(index_name)(encode_type)(index_value)(lower_bound)(upper_bound)(limit)(reverse)(show_payer) )
FC_REFLECT( eosio::chain_apis::read_only::get_table_rows_result, (rows)(more)(next_key)(next_key_bytes) );

FC_REFLECT( eosio::chain_apis::read_only::get_table_by_scope_params, (code)(table)(lower_bound)(upper_bound)(limit)(reverse) )
FC_REFLECT( eosio::chain_apis::read_only::get_table_by_scope_result_row, (code)(scope)(table)(payer)(count));
FC_REFLECT( eosio::chain_apis::read_only::get_table_by_scope_result, (rows)(more) );

FC_REFLECT( eosio::chain_apis::read_only::get_currency_balance_params, (code)(account)(symbol));
FC_REFLECT( eosio::chain_apis::read_only::get_currency_stats_params, (code)(symbol));
FC_REFLECT( eosio::chain_apis::read_only::get_currency_stats_result, (supply)(max_supply)(issuer));

FC_REFLECT( eosio::chain_apis::read_only::get_producers_params, (json)(lower_bound)(limit) )
FC_REFLECT( eosio::chain_apis::read_only::get_producers_result, (rows)(total_producer_vote_weight)(more) );

FC_REFLECT_EMPTY( eosio::chain_apis::read_only::get_producer_schedule_params )
FC_REFLECT( eosio::chain_apis::read_only::get_producer_schedule_result, (active)(pending)(proposed) );

FC_REFLECT( eosio::chain_apis::read_only::get_scheduled_transactions_params, (json)(lower_bound)(limit) )
FC_REFLECT( eosio::chain_apis::read_only::get_scheduled_transactions_result, (transactions)(more) );

FC_REFLECT( eosio::chain_apis::read_only::account_resource_info, (used)(available)(max)(last_usage_update_time)(current_used) )
FC_REFLECT( eosio::chain_apis::read_only::get_account_results,
            (account_name)(head_block_num)(head_block_time)(privileged)(last_code_update)(created)
            (core_liquid_balance)(ram_quota)(net_weight)(cpu_weight)(net_limit)(cpu_limit)(ram_usage)(permissions)
            (total_resources)(self_delegated_bandwidth)(refund_request)(voter_info)(rex_info) )
// @swap code_hash
FC_REFLECT( eosio::chain_apis::read_only::get_code_results, (account_name)(code_hash)(wast)(wasm)(abi) )
FC_REFLECT( eosio::chain_apis::read_only::get_code_hash_results, (account_name)(code_hash) )
FC_REFLECT( eosio::chain_apis::read_only::get_abi_results, (account_name)(abi) )
FC_REFLECT( eosio::chain_apis::read_only::get_account_params, (account_name)(expected_core_symbol) )
FC_REFLECT( eosio::chain_apis::read_only::get_code_params, (account_name)(code_as_wasm) )
FC_REFLECT( eosio::chain_apis::read_only::get_code_hash_params, (account_name) )
FC_REFLECT( eosio::chain_apis::read_only::get_abi_params, (account_name) )
FC_REFLECT( eosio::chain_apis::read_only::get_raw_code_and_abi_params, (account_name) )
FC_REFLECT( eosio::chain_apis::read_only::get_raw_code_and_abi_results, (account_name)(wasm)(abi) )
FC_REFLECT( eosio::chain_apis::read_only::get_raw_abi_params, (account_name)(abi_hash) )
FC_REFLECT( eosio::chain_apis::read_only::get_raw_abi_results, (account_name)(code_hash)(abi_hash)(abi) )
FC_REFLECT( eosio::chain_apis::read_only::producer_info, (producer_name) )
FC_REFLECT( eosio::chain_apis::read_only::abi_json_to_bin_params, (code)(action)(args) )
FC_REFLECT( eosio::chain_apis::read_only::abi_json_to_bin_result, (binargs) )
FC_REFLECT( eosio::chain_apis::read_only::abi_bin_to_json_params, (code)(action)(binargs) )
FC_REFLECT( eosio::chain_apis::read_only::abi_bin_to_json_result, (args) )
FC_REFLECT( eosio::chain_apis::read_only::get_required_keys_params, (transaction)(available_keys) )
FC_REFLECT( eosio::chain_apis::read_only::get_required_keys_result, (required_keys) )
FC_REFLECT( eosio::chain_apis::read_only::send_ro_transaction_params_v1, (return_failure_traces)(transaction) )
FC_REFLECT( eosio::chain_apis::read_only::send_ro_transaction_results, (head_block_num)(head_block_id)(last_irreversible_block_num)(last_irreversible_block_id)(code_hash)(pending_transactions)(result) )
FC_REFLECT( eosio::chain_apis::read_only::get_all_accounts_params, (page)(page_size))
FC_REFLECT( eosio::chain_apis::read_only::get_all_accounts_result::account_result, (name)(creation_date))
FC_REFLECT( eosio::chain_apis::read_only::get_all_accounts_result, (accounts))

