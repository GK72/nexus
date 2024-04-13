use std::path::Path;

fn main() {
    let dst = cmake::build(Path::new(".").canonicalize().unwrap());
    println!("cargo::rustc-link-search=native={}/lib", dst.display());
    println!("cargo::rustc-link-lib=static=utils");
}