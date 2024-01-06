use std::io::Read;

type Job = Box<dyn FnOnce() + Send + 'static>;

#[allow(dead_code)]
struct Worker {
    id: usize,
    thread: std::thread::JoinHandle<()>,
}

impl Worker {
    fn new(id: usize, receiver: std::sync::Arc<std::sync::Mutex<std::sync::mpsc::Receiver<Job>>>) -> Self {
        let thread = std::thread::spawn(move || loop {
            let job = receiver.lock().unwrap().recv().unwrap();
            job();
        });

        return Self { id, thread };
    }
}

pub struct ThreadPool {
    #[allow(dead_code)]
    workers: Vec<Worker>,

    sender: std::sync::mpsc::Sender<Job>,
}

impl ThreadPool {
    pub fn new(size: usize) -> Self {
        assert!(size > 0);

        let (sender, receiver) = std::sync::mpsc::channel();
        let receiver = std::sync::Arc::new(std::sync::Mutex::new(receiver));
        let mut workers = Vec::with_capacity(size);

        for id in 0..size {
            workers.push(Worker::new(id, std::sync::Arc::clone(&receiver)));
        }

        return Self { workers, sender };
    }

    pub fn execute<F>(&self, f: F)
    where
        F: FnOnce() + std::marker::Send + 'static,
    {
        let job = Box::new(f);
        return self.sender.send(job).unwrap();
    }
}

type ConnectionHandler = Box<dyn Fn(&[u8])>;

struct Connection {
    stream: std::net::TcpStream,
    buffer: [u8; 64],               // TODO(feat): ring-buffer
    handler: ConnectionHandler,
}

impl Connection {
    fn new<F>(stream: std::net::TcpStream, handler: F) -> Self
    where
        F: Fn(&[u8]) + 'static
    {
        return Self {
            stream,
            buffer: [0; 64],
            handler: Box::new(handler),
        };
    }

    fn read_some(&mut self) -> usize {
        match self.stream.read(&mut self.buffer) {
            Ok(bytes) => {
                let buf = self.buffer[0..bytes].to_vec();
                (self.handler)(&buf);
                return bytes;
            }
            Err(e) => {
                println!("Failed to read: {}", e);
                return 0;
            }
        }
    }
}

pub fn start_server<F>(address: &String, connection_handler: F)
where
    F: Fn(&[u8]) + std::marker::Send + std::marker::Sync + 'static + Copy
{
    let listener = std::net::TcpListener::bind(&address)
        .expect("Failed to bind to address");

    let pool = ThreadPool::new(4);

    println!("Server listening on {}", &address);

    for stream in listener.incoming() {
        let stream = stream.unwrap();

        pool.execute(move || {
            let mut connection = Connection::new(stream, connection_handler);
            while connection.read_some() > 0 {}
        });
    }
}
