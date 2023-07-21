#![allow(non_snake_case)]
use std::env;
use std::fs;
use std::collections::HashMap;
use std::io::BufReader;
use std::fs::File;
use std::io::prelude::*;


static EXTRA_INFO_FILE: &str = "extr_info_example";
static SEVER_DESC_FILE: &str = "server_descriptors_example";


struct Bridge {
    name: String,
	router: String,
	master_key: String,	
    dirreq_v3_ips: HashMap<String,u64>    
}

struct Dector {
	pub bridge_db: Vec<Bridge>,
}

impl Dector {	
	pub fn init(&mut self) {
		self.parse_bstats_file();
	}

	pub fn update(&mut self) {

	}

	fn parse_sever_descriptors_file(&mut self) {

	}

	fn parse_bstats_file(&mut self) {
		println!("Parsing example bridge stats file");
		
		let mut result = Vec::new();
		let file = File::open(EXTRA_INFO_FILE).expect("could not open file");
		let mut buf_reader = BufReader::new(file);
		let mut contents = String::new();
		buf_reader.read_to_string(&mut contents).expect("could not read file");
		// print!("{}", contents);
		
		let mut bridgeId = String::new();
		let mut bridgeIPs:HashMap<String,u64> = HashMap::new();
		let mut nextBridge = false;
		let mut bridgeStatLineIndex = 0;
		for line in contents.lines() {
			result.push(line.to_string());
			if !nextBridge {
				if line.contains("@type ") {
					bridgeStatLineIndex = 1;
					nextBridge = true;
				}			
			}
			else {
				if bridgeStatLineIndex == 1 {
					let tokens:Vec<&str> = line.split(" ").collect();
					bridgeId = tokens[2].to_string();
					bridgeStatLineIndex = 0;
				}
				if line.contains("dirreq-v3-ips") {
					let tokens:Vec<&str> = line.split(" ").collect();
					if !tokens[1].eq("") {
						println!("{}", tokens[1]);
						let statsTokens:Vec<&str> = tokens[1].split(",").collect();
						for st in statsTokens {
							let regTokens:Vec<&str> = st.split("=").collect();
							let num:u64 = regTokens[1].parse().unwrap();
							bridgeIPs.insert(regTokens[0].to_string(), num);
						}

						println!("{}", bridgeId);

						for (k,v) in &bridgeIPs {
							print!("{}={}, ", k, v);
						}
						println!("");
						self.bridge_db.push(Bridge{id:bridgeId.clone(), dirreq_v3_ips:bridgeIPs.clone()});
					}
					nextBridge = false;
				}
			}
		}
		
		// for line in result {
		// 	println!("{}", line);
		// }

		println!("Done parsing example bridge stats file");
	}
}

fn main() {
    println!("Proof of concept simple test.");

	let mut d = Dector { bridge_db: Vec::new()};
	d.init();
	d.update();
}
