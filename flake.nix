{
  description = "endless-sky environment";
  nixConfig.bash-prompt = "[nix(endless-sky)] % ";
  inputs = { nixpkgs.url = "github:nixos/nixpkgs/23.11"; };
  outputs = { self, nixpkgs }:
    let
      armPkgs = nixpkgs.legacyPackages.aarch64-darwin.pkgs;
      armFrameworks = armPkgs.darwin.apple_sdk.frameworks;
      x86Pkgs = nixpkgs.legacyPackages.x86_64-darwin.pkgs;
      x86Frameworks = x86Pkgs.darwin.apple_sdk.frameworks;
    in {
      devShells.aarch64-darwin.default = armPkgs.mkShell {
        name = "endless-sky dev environment";
        packages = [
          armFrameworks.ApplicationServices
          armFrameworks.AudioToolbox
          armFrameworks.AudioUnit
          armFrameworks.CoreAudio
          armPkgs.SDL2
          armPkgs.cmake
          armPkgs.libcxx
          armPkgs.libjpeg
          armPkgs.libmad
          armPkgs.libmysqlconnectorcpp
          armPkgs.libpng
          armPkgs.ninja
        ];
        shellHook = ''
          NIX_CFLAGS_COMPILE="-Wno-elaborated-enum-base $NIX_CFLAGS_COMPILE"
          cmake . --preset macos -DVCPKG_TARGET_TRIPLET=arm64-osx-dynamic -DCMAKE_OSX_ARCHITECTURES=arm64
          cmake --build . --preset macos-debug
        '';
      };
      devShells.x86_64-darwin.default = x86Pkgs.mkShell {
        name = "endless-sky dev environment";
        packages = [
          x86Frameworks.ApplicationServices
          x86Frameworks.AudioToolbox
          x86Frameworks.AudioUnit
          x86Frameworks.CoreAudio
          x86Pkgs.SDL2
          x86Pkgs.cmake
          x86Pkgs.libcxx
          x86Pkgs.libjpeg
          x86Pkgs.libmad
          x86Pkgs.libmysqlconnectorcpp
          x86Pkgs.libpng
          x86Pkgs.ninja
        ];
        shellHook = ''
          NIX_CFLAGS_COMPILE="-Wno-elaborated-enum-base $NIX_CFLAGS_COMPILE"
          cmake . --preset macos
          cmake --build . --preset macos-debug
        '';
      };
    };
}
