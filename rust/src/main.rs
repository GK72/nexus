use server::start_server;

pub mod server;

fn handler(buffer: &[u8]) {
    dbg!(buffer);
}

fn main() {
    let args: Vec<String> = std::env::args().collect();

    let address: String = (|args: &Vec<String>| {
        if args.len() != 2 {
            return String::from("0.0.0.0:9999");
        }
        return args[1].clone();
    })(&args);

    start_server(&address, handler);
}
