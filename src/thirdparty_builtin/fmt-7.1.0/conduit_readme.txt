2020-11-12

Header-only source extracted from the fmt 7.1.0 release tarball

https://github.com/fmtlib/fmt/releases/tag/7.1.0

Added header (fmt/conduit_fmt.h) to simplify header only use 
(following path paved by  Axom)

Changed the default namespace from fmt to conduit_fmt to avoid potential symbol collisions

Fixed deprecated syntax by removing a space after operator"", thereby avoiding compile warnings

License: MIT
