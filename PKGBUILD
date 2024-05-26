# Maintainer: develux <erroor234@gmail.com>
pkgname=topmem-cpp-git
pkgver=WIP.r19.g90aa5eb
pkgrel=1
pkgdesc="It shows the top 10 processes using RAM and SWAP"
arch=('x86_64')
url="https://github.com/develux44/topmem-cpp"
license=('GPL-3.0-only')
makedepends=(gcc git)
conflicts=(topmem-cpp)
replaces=(topmem-cpp)
provides=("topmem-cpp=${pkgver}")
source=("git+https://github.com/develux44/topmem-cpp.git")
sha256sums=('SKIP')

pkgver() {
  cd topmem-cpp
  git describe --long --tags | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}


build() {
  cd topmem-cpp
  make
}

package() {
  cd topmem-cpp
  install -Dm755 topmem $pkgdir/usr/bin/topmem
}
