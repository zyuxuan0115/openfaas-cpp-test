use mongodb::{bson::doc,sync::Client};
use serde::{Deserialize, Serialize};
use OpenFaaSRPC::{make_rpc, get_arg_from_caller, send_return_value_to_caller,*};
use DbInterface::*;
use std::{fs::read_to_string, collections::HashMap, time::{SystemTime, Duration, Instant}};
use redis::{Commands, RedisResult};

fn main() {
  let input: String = get_arg_from_caller();
  let now = Instant::now();
  let user_id: i64 = serde_json::from_str(&input).unwrap();

  // get memcache connection
  let memcache_uri = get_memcached_uri();
  let memcache_client = memcache::connect(&memcache_uri[..]).unwrap();
  let mut user_id_str: String = user_id.to_string();
  user_id_str.push_str(":followees"); 
  let result_str: Option<String> = memcache_client.get(&user_id_str[..]).unwrap();
  let mut return_value: String = String::new();
  match result_str {
    Some(result) => {
      return_value = result;
    },
    None => {
      // get redis connection
      let redis_uri = get_redis_rw_uri();
      let redis_client = redis::Client::open(&redis_uri[..]).unwrap();
      let mut con = redis_client.get_connection().unwrap();

      let mut real_name: String = "social-graph:".to_string();
      real_name.push_str(&(user_id.to_string()));
      let followees_str_redis_result: redis::RedisResult<String> = con.hget(&real_name[..], "followees");
      match followees_str_redis_result {
        Ok(followees_str) => {
          return_value = followees_str;
        },
        Err(_) => {
          println!("user_id: {} not found", user_id);
          panic!("user_id: {} not found", user_id);
        },
      }
    },
  }
  let followees_timestamp: Vec<Followee> = serde_json::from_str(&return_value).unwrap();
  let followees: Vec<i64> = followees_timestamp.into_iter().map(|x| x.followee_id).collect();
  let serialized = serde_json::to_string(&followees).unwrap();
  let new_now =  Instant::now();
  println!("{:?}", new_now.duration_since(now));
  send_return_value_to_caller(serialized);
}

