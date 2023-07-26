{ stdenv
, stdenvNoCC
, lib
, fetchurl
, fetchFromGitHub
, autoPatchelfHook
, python310
, writeText
, cmake
, ninja
, openssl
, rustPlatform
, libusb1
, zlib
, darwin
, libiconv
}:
let
  version = "v5.0.1";

  src = fetchFromGitHub {
    owner = "espressif";
    repo = "esp-idf";
    rev = version;
    hash = "sha256-kyCEoA8synodDfYdN8gq2/ezacxz5DFOD9wrPDZC89U=";
    fetchSubmodules = true;
  };

  python = python310.override {
    packageOverrides = final: prev:
      let
        pypiPackage = pname: version: hash: deps:
          final.buildPythonPackage {
            inherit pname version;
            propagatedBuildInputs = deps;
            src = final.fetchPypi {
              inherit pname version hash;
            };
            doCheck = false;
          };
      in
      {
        idf-component-manager = pypiPackage "idf_component_manager" "1.2.2"
          "sha256-NnLz4zpNERAFsaeuFhKs0RnYUdIU+uqwVOi7EPY4ByA="
          (with final; [
            cachecontrol
            click
            future
            pyyaml
            packaging
            requests-file
            colorama
            lockfile
            requests-toolbelt
            schema
            tqdm
          ]);
        esp-coredump = pypiPackage "esp-coredump" "1.4.2"
          "sha256-sAZQOxgm/kzj4+XaO6UvvtZMr89eP3FER8jkSwDLkvM="
          (with final; [ construct esptool pygdbmi ]);
        esptool = pypiPackage "esptool" "4.5.1"
          "sha256-4+tZg2Ej5ev3k+9jkxH32FZFUmSH2LHCtRFZtFUQa5o="
          (with final; [ bitstring cryptography ecdsa pyserial reedsolo ]);
        esp-idf-kconfig = pypiPackage "esp-idf-kconfig" "1.1.0"
          "sha256-s8ZXt6cf5w2pZSxQNIs/SODAUvHNgxyQ+onaCa7UbFA="
          (with final; [ kconfiglib ]);
        esp-idf-monitor = pypiPackage "esp-idf-monitor" "1.0.0"
          "sha256-CrdQI4Lp7TQvmolf3aHuokrQlnrPneemMisG9TGcTls="
          (with final; [ esp-coredump pyelftools ]);
        freertos-gdb = pypiPackage "freertos-gdb" "1.0.2"
          "sha256-o0ZoTy7OLVnrhSepya+MwaILgJSojs2hfmI86D9C3cs=" [ ];
        click = pypiPackage "click" "8.0.4"
          "sha256-hFjXsSh8X7EoyQ4jOBz5nc3nS+r2x/9jhM6E1v4JCts=" [ ];
        future = pypiPackage "future" "0.18.2"
          "sha256-sb6tkLcM9uw/BxCuU6UlNg+jYNMGqGWDrca/g6TbU30=" [ ];
        pyelftools = pypiPackage "pyelftools" "0.27"
          "sha256-zehU5mJ3TFRX1ojKQWFfZZQYe6cGevEBIy34iaa3pms=" [ ];
        cryptography = final.buildPythonPackage rec {
          pname = "cryptography";
          version = "36.0.2";
          src = final.fetchPypi {
            inherit pname version;
            hash = "sha256-cPj097sqyfNAZVy6yJ1oxSevW7Q4dSKoQT6EHj5mKMk=";
          };
          doCheck = false;
          cargoRoot = "src/rust";
          cargoDeps = rustPlatform.fetchCargoTarball {
            inherit src;
            sourceRoot = "${pname}-${version}/${cargoRoot}";
            name = "${pname}-${version}";
            hash = "sha256-6C4N445h4Xf2nCc9rJWpSZaNPilR9GfgbmKvNlSIFqg=";
          };
          nativeBuildInputs = with final; [
            cffi
            rustPlatform.cargoSetupHook
            setuptools-rust
          ] ++ (with rustPlatform; [ rust.cargo rust.rustc ]);
          buildInputs = [ openssl ]
            ++ lib.optionals stdenv.isDarwin [
            darwin.apple_sdk.frameworks.Security
            libiconv
          ];
          propagatedBuildInputs = [ final.cffi ];
        };
        cherrypy = pypiPackage "CherryPy" "18.8.0"
          "sha256-m0jPuoovFtW2QZzGV+bVHbAFujXF44JORyi7A7vH75s="
          (with final; [
            cheroot
            jaraco_collections
            more-itertools
            portend
            setuptools-scm
            zc_lockfile
          ]);
      };
  };

  python-env = python.withPackages (ps: with ps; [
    setuptools
    click
    pyserial
    cryptography
    pyparsing
    pyelftools
    idf-component-manager
    esp-coredump
    esptool
    esp-idf-kconfig
    esp-idf-monitor
    freertos-gdb
  ]);

  idf-tools-dir = stdenv.mkDerivation {
    name = "idf-tools";
    dontUnpack = true;
    installPhase = ''
      mkdir -p $out/python_env
      cp ${./espidf.constraints.v5.0.txt} $out/espidf.constraints.v5.0.txt
      ln -s ${python-env} $out/python_env/idf5.0_py3.10_env
    '';
  };

  idf-platform = {
    x86_64-linux = "linux-amd64";
    aarch64-linux = "linux-arm64";
    armv7l-linux = "linux-armel";
    x86_64-darwin = "macos";
    aarch64-darwin = "macos-arm64";
  }.${stdenv.hostPlatform.system};

  getIdfToolSrc = name: version: lib.pipe "${src}/tools/tools.json" [
    builtins.readFile
    builtins.fromJSON
    (x: x.tools)
    (lib.findFirst (e: e.name == name) { })
    (x: x.versions)
    (lib.findFirst (e: e.name == version) { })
    (x: x.${idf-platform})
    (x: fetchurl { inherit (x) url sha256; })
  ];

  getIdfToolPkg =
    { pname
    , version
    , dir ? pname
    , libs ? [ ]
    , ignored ? [ ]
    , mainProgram ? null
    }: stdenvNoCC.mkDerivation rec {
      inherit pname version;
      src = getIdfToolSrc pname version;
      dontConfigure = true;
      dontBuild = true;
      dontStrip = true;
      nativeBuildInputs = lib.optionals stdenv.isLinux [ autoPatchelfHook ];
      buildInputs = libs;
      autoPatchelfIgnoreMissingDeps = ignored;
      installPhase = "cp -r . $out";
      meta = lib.optionalAttrs (mainProgram != null) { inherit mainProgram; };
    };
in
stdenvNoCC.mkDerivation rec {
  pname = "esp-idf";
  inherit version src;

  passthru.pkgs = {
    python = python-env;
    xtensa-esp-elf-gdb = getIdfToolPkg {
      pname = "xtensa-esp-elf-gdb";
      version = "11.2_20220823";
      libs = [ python ];
      ignored = [ "libpython*" ];
    };
    xtensa-esp32-elf = getIdfToolPkg {
      pname = "xtensa-esp32-elf";
      version = "esp-2022r1-11.2.0";
      libs = [ stdenv.cc.cc ];
      ignored = [ "libpython2.7.so.1.0" ];
    };
    openocd-esp32 = getIdfToolPkg {
      pname = "openocd-esp32";
      version = "v0.11.0-esp32-20221026";
      libs = [ libusb1 zlib ];
      mainProgram = "openocd";
    };
    xtensa-clang = getIdfToolPkg {
      pname = "xtensa-clang";
      version = "14.0.0-38679f0333";
      dir = "xtensa-esp32-elf-clang";
      libs = [ stdenv.cc.cc zlib ];
      ignored = [ "libpython2.7.so.1.0" ];
    };
  };

  propagatedBuildInputs = with passthru.pkgs; [
    xtensa-esp-elf-gdb
    xtensa-esp32-elf
    openocd-esp32
  ];

  buildInputs = [ python-env ];

  idfIncludePaths = [
    "tools"
    "components/esptool_py/esptool"
    "components/espcoredump"
    "components/partition_table"
    "components/app_update"
    "components/nvs_flash/nvs_partition_generator"
  ];

  installPhase = "cp -r . $out";

  setupHook = writeText "setup-hook.sh" ''
    export IDF_PATH=@out@
    export IDF_TOOLS_PATH=${idf-tools-dir}
    for inc in @idfIncludePaths@; do
      export PATH=$IDF_PATH/$inc:$PATH
    done
    # Directly add to path instead of inputs to avoid propogating setupHooks
    export PATH=${cmake}/bin:${ninja}/bin:$PATH
  '';
}
