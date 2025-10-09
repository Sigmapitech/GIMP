{
  description = "Pretty's development flake - portable, reliable, ergonimic - TTY";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    supportedSystems = ["x86_64-linux"];
    forAllSystems = f: nixpkgs.lib.genAttrs
      supportedSystems (system: f nixpkgs.legacyPackages.${system});
  in {
    formatter = forAllSystems (pkgs: pkgs.alejandra);

    packages = forAllSystems (pkgs: {
      default = self.packages.${pkgs.system}.epi-gimp;

      epi-gimp = pkgs.callPackage ./epi-gimp.nix { };
    });

    devShells = forAllSystems (pkgs: {
      default = pkgs.mkShell {
        env.CC = pkgs.stdenv.cc;
        inputsFrom = [ self.packages.${pkgs.system}.epi-gimp ];

        hardeningDisable = [ "format" ];
        packages = with pkgs; [
          compiledb
          valgrind
        ];
      };
    });
  };
}
