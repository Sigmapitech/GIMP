{
  lib,
  stdenv,
  pkg-config,
  gtk4,
}:
stdenv.mkDerivation {
  name = "epi-gimp";
  src = ./.;

  hardeningDisable = ["format"];

  nativeBuildInputs = [pkg-config];

  buildInputs = [
    gtk4
  ];

  env = {
    PREFIX = "${placeholder "out"}";
    RELEASE = true;
  };

  meta = {
    description = "GIMP re-creation project as part of the SPW";
    maintainers = with lib.maintainers; [sigmanificient];
    license = lib.licenses.bsd3;
    mainProgram = "epi-gimp";
  };
}
