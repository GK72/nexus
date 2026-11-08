# frozen_string_literal: true

# Part of Nexus Library
#
# Plugin: live reload for `tools/doc.sh --serve`, which sets `nxs-live`. The page asks the server
# every second when its file last changed, and reloads when it did. Pages rendered without
# `--serve` carry nothing.

require 'asciidoctor'
require 'asciidoctor/extensions'

module Nxs
  class LiveDocinfo < Asciidoctor::Extensions::DocinfoProcessor
    use_dsl
    at_location :footer

    SCRIPT = <<~JS
      <script>
      (() => {
        let seen = null;
        const check = async () => {
          try {
            const r = await fetch(location.pathname, { method: "HEAD", cache: "no-store" });
            const stamp = r.headers.get("Last-Modified");
            if (seen && stamp && stamp !== seen) { try { sessionStorage.setItem("nx-live-y", scrollY); } catch {} location.reload(); return; }
            seen = stamp;
          } catch { /* server stopped: keep the page */ }
          setTimeout(check, 1000);
        };
        try {
          const y = sessionStorage.getItem("nx-live-y");
          if (y !== null) { sessionStorage.removeItem("nx-live-y"); addEventListener("load", () => scrollTo(0, +y)); }
        } catch { /* no storage: start at the top */ }
        check();
      })();
      </script>
    JS

    def process(document)
      SCRIPT if document.attr?('nxs-live')
    end
  end
end

Asciidoctor::Extensions.register do
  docinfo_processor Nxs::LiveDocinfo
end
