use mongodb::{bson::doc,sync::Client,options::UpdateOptions};
use serde::{Deserialize, Serialize};
use OpenFaaSRPC::{make_rpc, get_arg_from_caller, send_return_value_to_caller,*};
use DbInterface::*;
use std::{fs::read_to_string, collections::HashMap};
use redis::{Commands};

fn main() {
  let input: String = get_arg_from_caller();
  let timeline_info: write_user_timeline_get = serde_json::from_str(&input).unwrap();

  // connect to mongodb
  let uri = get_mongodb_uri();
  let client = Client::with_uri_str(&uri[..]).unwrap();
  let database = client.database("user-timeline");
  let collection = database.collection::<user_timeline_entry>("user-timeline");

  // update mongodb
  let query = doc!{"user_id":timeline_info.user_id};
  let update = doc!{"$push":doc!{"posts":doc!{"$each":[doc!{"post_id":timeline_info.post_id,"timestamp":timeline_info.timestamp}], "$position":0}}};

  let options = UpdateOptions::builder()
        .upsert(Some(true))
        .build();

  let res = collection.update_many(query, update, options).unwrap();

  // update redis
  let redis_uri = get_redis_rw_uri();
  let redis_client = redis::Client::open(&redis_uri[..]).unwrap();
  let mut con = redis_client.get_connection().unwrap();
   
  let user_id_str = timeline_info.user_id.to_string(); 
  let post_id_str = timeline_info.post_id.to_string();
  let res: isize = con.zadd(&user_id_str[..], &post_id_str[..], timeline_info.timestamp).unwrap();

  send_return_value_to_caller("".to_string());
}

