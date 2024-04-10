use mongodb::{bson::doc,sync::Client};
use serde::{Deserialize, Serialize};
use OpenFaaSRPC::{make_rpc, get_arg_from_caller, send_return_value_to_caller};
use std::{fs::read_to_string, collections::HashMap};
use memcache::Client as memcached_client;

#[derive(Debug, Serialize, Deserialize)]
struct user_mention {
  user_id: i64,
  user_name: String,
}


fn read_lines(filename: &str) -> Vec<String> {
    read_to_string(filename) 
        .unwrap()  // panic on possible file-reading errors
        .lines()  // split the string into an iterator of string slices
        .map(String::from)  // make each slice into a string
        .collect()  // gather them together into a vector
}
 

fn get_mongodb_uri() -> String{
  let passwords: Vec<String> = read_lines("/var/openfaas/secrets/mongo-db-password");
  if passwords.len() == 0 {
    println!("no password found!");
  } 
  if passwords.len() > 1 {
    println!("more than 1 passwords found!");
  }
  let password = passwords[0].to_owned();
  let mut uri: String = String::from("mongodb://root:");
  uri.push_str(&password[..]);
  uri.push_str("@mongodb.default.svc.cluster.local:27017");
  uri
}

fn get_memcached_uri() -> String {
  "memcache://sn-memcache-memcached.default.svc.cluster.local:11211??timeout=10&tcp_nodelay=true".to_string()
}

fn remove_suffix<'a>(s: &'a str, suffix: &str) -> &'a str {
    match s.strip_suffix(suffix) {
        Some(s) => s,
        None => s
    }
}

fn main() {
  let input: String = get_arg_from_caller();
  let usernames: Vec<String> = serde_json::from_str(&input).unwrap();

  let uri = get_mongodb_uri();
  let client = Client::with_uri_str(&uri[..]).unwrap();
  let database = client.database("user");
  let collection = database.collection::<user_mention>("user");

  let memcache_uri = get_memcached_uri();
  let memcache_client = memcache::connect(&memcache_uri[..]).unwrap();  
  
  let mut usernames_not_cached: HashMap<String, bool> = HashMap::new();
  for username in &usernames {
    let mut usern: String = (&username[..]).to_string();
    usern.push_str(":user_id");
    memcache_client.set(&usern[..], 16, 0).unwrap();
    usernames_not_cached.insert((&username[..]).to_string(), false);
  }

  let usernames_suffix: Vec<String> = usernames.iter().map(|x|{let mut y = x.to_string(); y.push_str(":user_id"); y}).collect();
  let usernames_str: Vec<&str> = usernames_suffix.iter().map(|x|&x[..]).collect();
  let usernames_array: &[&str] = &usernames_str;
  let result: HashMap<String, i64> = memcache_client.gets(&usernames_array).unwrap();
  let mut user_mentions: Vec<user_mention> = Vec::new();
  for (key, value) in &result {
    let username: String = remove_suffix(&key[..], ":user_id").to_string();
    usernames_not_cached.remove(&username);
    let new_user_mention = user_mention {
      user_id: value.to_owned(),
      user_name: username,
    };
    user_mentions.push(new_user_mention);
  }



  let serialized = serde_json::to_string(&user_mentions).unwrap();
  send_return_value_to_caller(serialized);
}

