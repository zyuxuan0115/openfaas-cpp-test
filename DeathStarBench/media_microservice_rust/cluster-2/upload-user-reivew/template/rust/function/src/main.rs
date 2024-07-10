use mongodb::{bson::doc,sync::Client};
use OpenFaaSRPC::{make_rpc, get_arg_from_caller, send_return_value_to_caller,*};
use DbInterface::*;
use std::{fs::read_to_string, collections::HashMap, time::{SystemTime,Duration, Instant}};
use redis::{Commands};

fn main() {
  let input: String = get_arg_from_caller();
//  let now = Instant::now();
  let review_info: UploadUserReviewArgs = serde_json::from_str(&input).unwrap();

  let time_stamp = SystemTime::now().duration_since(SystemTime::UNIX_EPOCH).unwrap().as_secs();

  let uri = get_mongodb_uri();
  let client = Client::with_uri_str(&uri[..]).unwrap();
  let database = client.database("user-review");
  let collection = database.collection::<UserReviewEntry>("user-review");

  let query = doc!{"user_id":review_info.user_id};
  let mongodb_result = collection.find_one(query, None).unwrap();
  
  match mongodb_result {
    Some(x) => {
      let search_query = doc!{"user_id":review_info.user_id};
      let update_query = doc!{"$push":doc!{"reviews":doc!{"$each":[{"review_id":review_info.user_id,"timestamp":review_info.timestamp}],"$position":0}}};
      let res = collection.update_one(search_query, update_query, None).unwrap();
    },
    None => {
      let review = Review {
        review_id: review_info.review_id,
        timestamp: review_info.timestamp,
      };
      let new_user_review = UserReviewEntry {
        user_id: review_info.user_id,
        reviews: vec![review],
      };
      collection.insert_one(new_user_review, None).unwrap();
    }
  }


  let redis_uri = get_redis_rw_uri();
  let redis_client = redis::Client::open(&redis_uri[..]).unwrap();
  let mut con = redis_client.get_connection().unwrap();

//  let (k1, k2) : (isize, isize) = redis::pipe()
//                                     .cmd("ZREM").arg(&user_id_str[..]).arg(follow_info.followee_id)
//                                     .cmd("ZREM").arg(&followee_id_str[..]).arg(follow_info.user_id)
//                                     .query(&mut con).unwrap();

//  let new_now =  Instant::now();
//  println!("SocialGraphUnfollow: {:?}", new_now.duration_since(now));
  send_return_value_to_caller("".to_string());
}

