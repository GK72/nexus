use nrs::tcp_server;

fn handler(buffer: &[u8]) {
    if buffer.len() == 0 {
        return;
    }

    println!("{:?}", buffer);
}

fn main() {
    let args: Vec<String> = std::env::args().collect();

    let address: String = (|args: &Vec<String>| {
        if args.len() != 2 {
            return String::from("0.0.0.0:9999");
        }
        return args[1].clone();
    })(&args);

    tcp_server::start(&address, handler);
}
