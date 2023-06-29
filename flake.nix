{
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-22.11";
  };
  outputs = { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "armv7l-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      eachSystem = f: with nixpkgs.lib; foldAttrs mergeAttrs { }
        (map (s: mapAttrs (_: v: { ${s} = v; }) (f s)) systems);
    in
    eachSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        inherit (pkgs) lib fd;
        runCheck = cmd: pkgs.runCommand "check" { }
          "cp --no-preserve=mode -r ${./.} src; cd src\n${cmd}\n touch $out";
        format_tools = with pkgs; [
          clang-tools
          coreutils
          cmake-format
          nixpkgs-fmt
          nodePackages.prettier
        ];
        format_cmd = pkgs.writeShellScript "format" ''
          PATH=${lib.makeBinPath format_tools}
          case "$1" in
            *.c | *.h)
              clang-format -i "$1";;
            *.cmake | */CMakeLists.txt)
              cmake-format -c ${./.cmake-format.yml} -i "$1";;
            *.nix)
              nixpkgs-fmt "$1";;
            *.md | *.json | *.yml)
              prettier --write "$1";;
          esac &>/dev/null
        '';
        getFetchContentFlags = file:
          let
            inherit (builtins) head elemAt match;
            parse = match
              "(.*)\nFetchContent_Declare\\(\n  ([^\n]*)\n([^)]*)\\).*"
              file;
            name = elemAt parse 1;
            content = elemAt parse 2;
            getKey = key: elemAt
              (match "(.*\n)?  ${key} ([^\n]*)(\n.*)?" content) 1;
            url = getKey "GIT_REPOSITORY";
            pkg = pkgs.fetchFromGitHub {
              owner = head (match ".*github.com/([^/]*)/.*" url);
              repo = head (match ".*/([^/]*)\\.git" url);
              rev = getKey "GIT_TAG";
              hash = getKey "# hash:";
            };
          in
          if (parse == null) then [ ] else
          ([ "-DFETCHCONTENT_SOURCE_DIR_${lib.toUpper name}=${pkg}" ] ++
            getFetchContentFlags (head parse));
      in
      rec {
        devShells.default = pkgs.mkShell {
          inputsFrom = [ packages.default ];
          packages = with pkgs; [ clang-tools jq moreutils ] ++ format_tools;
          hardeningDisable = [ "fortify" ];
        };
        packages.default = pkgs.stdenv.mkDerivation {
          name = "coreOTA";
          src = ./.;
          nativeBuildInputs = with pkgs; [ cmake ];
          buildInputs = with pkgs; [ openssl_1_1 jansson ];
          hardeningDisable = [ "fortify" ];
          cmakeFlags = getFetchContentFlags
            (builtins.readFile ./CMakeLists.txt);
          installPhase = "cp coreOTA $out";
        };
        checks = packages // {
          formatting = runCheck ''
            ${lib.getExe formatter} .
            ${pkgs.diffutils}/bin/diff -rq --no-dereference . ${./.} | \
              sed -e 's/Files \(.*\) and .* differ/\1 not formatted/'
          '';
          editorconfig = runCheck ''
            ${pkgs.editorconfig-checker}/bin/editorconfig-checker
          '';
          cmake-lint = runCheck ''
            ${pkgs.cmake-format}/bin/cmake-lint \
              $(${fd}/bin/fd '.*\.cmake|CMakeLists.txt' \
                -E 'reference/esp32/lib') -c .cmake-format.yml
          '';
        };
        formatter = pkgs.writeShellScriptBin "formatter" ''
          for f in "$@"; do
            if [ -d "$f" ]; then
              (cd "$f"; ${fd}/bin/fd -t f -H -E '**/lib/*/*' -x ${format_cmd})
            else
              ${format_cmd} "$f"
            fi
          done
        '';
      });
}
