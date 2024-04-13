use std::path::Path;

fn main() {
    if cfg!(target_os = "windows") {
        println!("cargo::warning=C++ bindings are disabled! TODO: make it work on Windows (cannot find `utils.cpp`)");
        return;
    }

    let dst = cmake::Config::new(Path::new(".").canonicalize().unwrap())
        .generator("Ninja")
        .build();

    println!("cargo::rustc-link-search=native={}/lib", dst.display());
    println!("cargo::rustc-link-lib=static=utils");
}
