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
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ (import ./nix/overlay.nix) ];
        };
        inherit (pkgs) lib;
        filterBin = pkg: selectedBin: pkgs.stdenvNoCC.mkDerivation {
          name = "clangd";
          dontUnpack = true;
          installPhase = ''
            mkdir -p $out/bin;
            ln -s "${pkg}/bin/${selectedBin}" "$out/bin/${selectedBin}"
          '';
        };
        clangdPackage =
          if (lib.any (s: s == system)
            [ "x86_64-linux" "x86_64-darwin" "aarch64-darwin" ])
          then pkgs.esp-idf.pkgs.xtensa-clang
          else pkgs.clang-tools;
      in
      rec {
        devShells.default = pkgs.mkShell {
          inputsFrom = [ packages.default ];
          packages = [
            pkgs.esp-idf.pkgs.python
            (filterBin clangdPackage "clangd")
          ];
        };
        packages = {
          inherit (pkgs) esp-idf;
          default = pkgs.stdenvNoCC.mkDerivation {
            name = "coreota_esp32_example.bin";
            src = ./.;
            buildInputs = with pkgs; [ esp-idf ];
            dontFixup = true;
            buildPhase = ''
              export HOME=$(pwd)
              idf.py build
            '';
            installPhase = ''
              cp build/coreota_esp32_example.bin $out
            '';
          };
        };
        formatter = pkgs.writeShellScriptBin "formatter" ''
          ROOT=$(while [ ! -f flake.nix ]; do cd ..; done; cd ../..; pwd)
          nix run $ROOT#formatter.${system} -- "$@"
        '';
      });
}
