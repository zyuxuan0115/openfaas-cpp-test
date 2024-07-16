use curl::easy::{Easy};
use std::{io::{self, Read, Write, BufReader}, error::Error, fs::{File, read_to_string}, path::Path, collections::HashMap};
use serde::{Deserialize, Serialize};

pub static maxSearchResults: usize = 5;

#[derive(Debug, Serialize, Deserialize)]
pub struct GetNearbyPointsRestArgs {
  pub latitude: f64,
  pub longitude: f64,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct GetNearbyPointsMusArgs {
  pub latitude: f64,
  pub longitude: f64,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct GetNearbyPointsCinemaArgs {
  pub latitude: f64,
  pub longitude: f64,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Restaurant {
  pub restaurant_id: String,
  pub latitude: f64,
  pub longitude: f64,
  pub restaurant_name: String,
  pub rating: f32,
  pub restaurant_type: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Museum {
  pub museum_id: String,
  pub latitude: f64,
  pub longitude: f64,
  pub museum_name: String,
  pub museum_type: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Cinema {
  pub cinema_id: String,
  pub latitude: f64, 
  pub longitude: f64,
  pub cinema_name: String, 
  pub cinema_type: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Point {
  pub id: String,
  pub latitude: f64,
  pub longitude: f64,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct FuncInfo{
  pub function_name: String,
  pub cluster_id: i64,
}

fn read_func_info_from_file<P: AsRef<Path>>(path: P) -> Result<Vec<FuncInfo>, Box<dyn Error>> {
  // Open the file in read-only mode with buffer.
  let file = File::open(path)?;
  let reader = BufReader::new(file);
 
  // Read the JSON contents of the file as an instance of `User`.
  let u: Vec<FuncInfo> = serde_json::from_reader(reader)?;
  Ok(u)
}

pub fn read_lines(filename: &str) -> Vec<String> {
  read_to_string(filename)
                 .unwrap()  // panic on possible file-reading errors
                 .lines()  // split the string into an iterator of string slices
                 .map(String::from)  // make each slice into a string
                 .collect()  // gather them together into a vector
}

pub fn make_rpc(func_name: &str, input: String) -> String {

  let func_vec = read_func_info_from_file("/home/rust/OpenFaaSRPC/func_info.json").unwrap();
  let func_info_hash: HashMap<String, i64> = func_vec.into_iter().map(|x| (x.function_name, x.cluster_id)).collect();

  let callee_cluster_id: i64 = func_info_hash.get(func_name).unwrap().to_owned();
  let mut easy = Easy::new();
  let mut url = String::new();

  let lines: Vec<String> = read_lines("/var/openfaas/secrets/ingress-enable");
  let ingress_enable = lines[0].clone();
  if ingress_enable == "0" {  
    url = match callee_cluster_id {
      1 => String::from("http://gateway.openfaas.svc.cluster.local.:8080/function/"),
      2 => String::from("http://gateway.openfaas2.svc.cluster.local.:8080/function/"),
      _ => {
        println!("Error: callee_cluster_id should not have other value");
        panic!("Error: callee_cluster_id should not have other value");
      },
    }
  }
  else {
    url = match callee_cluster_id {
      1 => String::from("http://ingress-nginx-controller.ingress-nginx.svc.cluster.local.:80/function/"),
      2 => String::from("http://ingress-nginx-controller.ingress-nginx2.svc.cluster.local.:80/function/"),
      _ => {
        println!("Error: callee_cluster_id should not have other value");
        panic!("Error: callee_cluster_id should not have other value"); 
      },
    }
  }
  let mut input_to_be_sent = (&input).as_bytes();
  url.push_str(func_name);
  easy.url(&url).unwrap();
  easy.post(true).unwrap();
  easy.post_field_size(input_to_be_sent.len() as u64).unwrap();

  let mut html_data = String::new();

  {
    let mut transfer = easy.transfer();
    transfer.read_function(|buf| {
      Ok(input_to_be_sent.read(buf).unwrap_or(0))
    }).unwrap();

    transfer.write_function(|data| {
      let data_str = String::from_utf8(Vec::from(data)).unwrap();
      html_data.push_str(&data_str);
      Ok(data.len())
    }).unwrap();

    transfer.perform().unwrap();
  }

  html_data
}

pub fn get_arg_from_caller() -> String{
  let mut buffer = String::new();
  let _ = io::stdin().read_line(&mut buffer);
  buffer
}

pub fn send_return_value_to_caller(output: String) -> (){
  let _ = io::stdout().write(&output[..].as_bytes());
}
