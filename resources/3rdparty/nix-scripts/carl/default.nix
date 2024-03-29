{ stdenv
, lib
, fetchFromGitHub
, autoconf
, pkg-config
, cmake
, cln
, ginac
, gmp
, boost
, eigen
, python3
, googletest
}:

let
  gtest-cmake = ./gtest.cmake;

in
stdenv.mkDerivation rec {
  pname = "carl";
  version = "14.27";

  buildInputs = [
    cln
    ginac
    gmp
    boost
    python3
    googletest
  ];

  nativeBuildInputs = [ autoconf pkg-config cmake ];

  propagatedBuildInputs = [ eigen ];

  src = fetchFromGitHub
    {
      owner = "moves-rwth";
      repo = "carl-storm";
      rev = "14.27";
      hash = "sha256-FMpL81yvcHvWLAMk4JyvxHKLpWXn0CV0Igb1BUuTTVQ=";
    };

  enableParallelBuilding = true;

  cmakeFlags = [
    "-DEXPORT_TO_CMAKE=off"
    "-DUSE_CLN_NUMBERS=on"
    "-DTHREAD_SAFE=on"
    "-DUSE_GINAC=on"
    "-DGINAC_FOUND=on"
    "-DGINAC_INCLUDE_DIR=${ginac}/include/ginac"
    "-DGINAC_LIBRARY=${ginac}/lib/libginac.so"
    "-DGTEST_FOUND=on"
    "-DGTEST_VERSION=${googletest.version}"
    "-DGTEST_MAIN_LIBRARY=${googletest}/lib/libgtest_main.a"
    "-DGTEST_LIBRARY=${googletest}/lib/libgtest.a"
  ];

  postPatch = ''
    cp ${gtest-cmake} resources/gtest.cmake
    substituteInPlace resources/gtest.cmake --subst-var-by googletest ${googletest}
    sed -e '/print_resource_info("GTest"/i include(resources/gtest.cmake)' -i resources/resources.cmake
  '';

  meta = with lib; {
    description = "Computer ARithmetic and Logic library";
    homepage = http://smtrat.github.io/carl;
    mainainers = [ maintainers.spacefrogg ];
    platforms = platforms.all;
  };
}
