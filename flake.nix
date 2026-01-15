{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    systems.url = "github:nix-systems/default";
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, systems, treefmt-nix, ... }:
    let
      pname = "m8c";
      version = "2.2.3";
      m8c-package =
        { stdenv
        , cmake
        , copyDesktopItems
        , pkg-config
        , sdl3
        , libserialport
        }:
        stdenv.mkDerivation {
          inherit pname version;
          src = ./.;

          nativeBuildInputs = [ cmake copyDesktopItems pkg-config ];
          buildInputs = [ sdl3 libserialport ];

          cmakeFlags = [ "-DSKIP_BUNDLE_FIXUP=ON" ];
        };
      m8c-sdl2-package =
        { stdenv
        , cmake
        , copyDesktopItems
        , pkg-config
        , SDL2
        , libserialport
        }:
        stdenv.mkDerivation {
          pname = "${pname}-sdl2";
          inherit version;
          src = ./.;

          nativeBuildInputs = [ cmake copyDesktopItems pkg-config ];
          buildInputs = [ SDL2 libserialport ];

          cmakeFlags = [ "-DUSE_SDL2=ON" "-DSKIP_BUNDLE_FIXUP=ON" ];
        };
      eachSystem = f: nixpkgs.lib.genAttrs (import systems) (system: f
        (import nixpkgs { inherit system; })
      );
      treefmtEval = eachSystem (pkgs: treefmt-nix.lib.evalModule pkgs (_: {
        projectRootFile = "flake.nix";
        programs = {
          clang-format.enable = false; # TODO(pope): Enable and format C code
          deadnix.enable = true;
          nixpkgs-fmt.enable = true;
          statix.enable = true;
        };
      }));
    in
    {
      packages = eachSystem (pkgs: rec {
        m8c = pkgs.callPackage m8c-package { };
        m8c-sdl2 = pkgs.callPackage m8c-sdl2-package { };
        default = m8c;
      });

      overlays.default = final: _prev: {
        inherit (self.packages.${final.system}) m8c m8c-sdl2;
      };

      apps = eachSystem (pkgs:
        let
          m8cBin = pkg:
            if pkgs.stdenv.isDarwin
            then "${pkg}/m8c.app/Contents/MacOS/m8c"
            else "${pkg}/bin/m8c";
        in rec {
          m8c = {
            type = "app";
            program = m8cBin self.packages.${pkgs.system}.m8c;
          };
          m8c-sdl2 = {
            type = "app";
            program = m8cBin self.packages.${pkgs.system}.m8c-sdl2;
          };
          default = m8c;
        });

      devShells = eachSystem (pkgs: with pkgs; {
        default = mkShell {
          packages = [
            cmake
            gnumake
            nix-prefetch-github
            treefmtEval.${system}.config.build.wrapper
          ];
          inputsFrom = [
            self.packages.${system}.m8c
            self.packages.${system}.m8c-sdl2
          ];
        };
      });

      formatter = eachSystem (pkgs: treefmtEval.${pkgs.system}.config.build.wrapper);
    };
}
