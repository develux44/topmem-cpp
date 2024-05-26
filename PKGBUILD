# Maintainer: develux <erroor234@gmail.com>
pkgname=topmem-cpp
pkgver=0.1
pkgrel=1
pkgdesc="It shows the top 10 processes using RAM and SWAP"
arch=('x86_64')
url="https://github.com/develux44/topmem-cpp"
license=('GPL-3.0-only')
makedepends=(gcc git)
source=("git+https://github.com/develux44/topmem-cpp.git#branch=main")
sha256sums=('SKIP')
build() {
  cd topmem-cpp
  make
}

package() {
  cd topmem-cpp
  install -D LICENSE -t "${pkgdir}/usr/share/licenses/${pkgname}"
}
