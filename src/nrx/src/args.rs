use std::collections::HashMap;

#[derive(Default, Debug)]
pub struct Arg {
    pub name: String,
    pub value: String,
    pub active: bool,
    pub required: bool,
    pub flag: bool
}

pub struct ArgParser {
    args_in: Vec<String>,
    args_map: HashMap<String, Arg>
}

impl Arg {
    pub fn new() -> Self {
        return Arg {
            name: String::new(),
            value: String::new(),
            active: false,
            required: false,
            flag: false
        };
    }
}

impl ArgParser {
    pub fn new(args: Vec<String>) -> Self {
        return Self{ args_in: args, args_map: HashMap::new() };
    }

    pub fn add(&mut self, arg_name: String) {
        let mut arg = Arg::new();
        arg.name = arg_name.clone();
        self.args_map.insert(arg_name, arg);
    }

    pub fn get(&self, arg_name: String, default: String) -> String {
        match self.args_map.get(&arg_name) {
            Some(x) => {
                if x.value == "" {
                    return default.clone();
                }
                else {
                    return x.value.clone();
                }
            },
            None => return default.clone(),
        }
    }

    pub fn process(&mut self) {
        for mut i in 0..self.args_in.len() {
            if let Some(arg) = self.args_map.get_mut(&self.args_in[i]) {
                arg.active = true;
                if !arg.flag {
                    i += 1;
                    arg.value = self.args_in[i].clone();
                }
            }
        }
    }
}