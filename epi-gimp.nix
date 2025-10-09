{
  lib,
  stdenv,
  pkg-config,
  gtk3,
  wrapGAppsHook3
}:

stdenv.mkDerivation {
  pname = "epi-gimp";
  version = "1.0";

  src = ./.;

  hardeningDisable = [ "format" ];

  nativeBuildInputs = [
    pkg-config
    wrapGAppsHook3
  ];

  buildInputs = [
    gtk3
  ];

  env = {
    PREFIX = "${placeholder "out"}";
    RELEASE = true;
  };

  meta = with lib; {
    description = "GIMP re-creation project as part of the SPW";
    maintainers = with maintainers; [ sigmanificient ];
    license = licenses.bsd3;
    mainProgram = "epi-gimp";
  };
}
