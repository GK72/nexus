pub struct Progress {
    total: usize,
    current: usize,
    start_time: std::time::Instant,
    last_update: std::time::Instant,
    elapsed: std::time::Duration,
    eta: std::time::Duration,
    pad: usize,
    bar_length_max: usize,
    bar_length_actual: usize
}

impl Progress {
    pub fn new(total: usize) -> Self {
        return Self {
            total,
            current: 0,
            start_time: std::time::Instant::now(),
            last_update: std::time::Instant::now(),
            elapsed: std::time::Duration::new(0, 0),
            eta: std::time::Duration::new(0, 0),
            pad: (total as f64).log10() as usize + 1,
            bar_length_max: 50,
            bar_length_actual: 0
        };
    }

    pub fn next(&mut self) -> &mut Self {
        self.current += 1;
        self.update_time();
        self.eta();
        self.update_bar();
        return self;
    }

    pub fn print(&self) -> std::io::Result<()> {
        use std::io::Write;

        if self.current != self.total {
            print!("{:>p$} / {:>p$}   {:?}\r", self.current, self.total, self.elapsed, p = self.pad);
            return std::io::stdout().flush();
        }
        else {
            print!("{:>p$} / {:>p$}   {:?}\n", self.current, self.total, self.elapsed, p = self.pad);
            return Ok(());
        }
    }

    pub fn print_bar(&self) -> std::io::Result<()> {
        use std::io::Write;

        if self.current != self.total {
            print!(" [{: <p$}]   {:.3}ms (ETA: {}s)\r",
                "=".repeat(self.bar_length_actual),
                self.elapsed.as_millis() as f64 / 1000.0,
                self.eta.as_secs(),
                p = self.bar_length_max
            );
            return std::io::stdout().flush();
        }
        else {
            print!(" [{: <p$}]   {:.3}ms (ETA: {}s)\n",
                "=".repeat(self.bar_length_actual),
                self.elapsed.as_millis() as f64 / 1000.0,
                self.eta.as_secs(),
                p = self.bar_length_max
            );
            return Ok(());
        }
    }

    fn eta(&mut self) -> std::time::Duration {
        let elapsed_ratio =  self.current as f64 / self.total as f64;
        self.eta = std::time::Duration::new(
            (1.0 / elapsed_ratio * self.elapsed.as_secs() as f64 - self.elapsed.as_secs() as f64) as u64,
            0
        );
        return self.eta;
    }

    fn update_time(&mut self) {
        let now = std::time::Instant::now();
        self.elapsed = now - self.start_time;
        self.last_update = now;
    }

    fn update_bar(&mut self) {
        self.bar_length_actual = (
            self.current as f64 / self.total as f64
            * self.bar_length_max as f64
        ) as usize
    }
}