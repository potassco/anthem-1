#!/bin/bash

build_directory=build/release-static
tmp_directory=/tmp

# build binary
mkdir -p $build_directory
pushd $build_directory
cmake ../.. -GNinja -DCMAKE_BUILD_TYPE=Release -DANTHEM_BUILD_STATIC=ON -DCLASP_BUILD_STATIC=ON -DCLINGO_BUILD_STATIC=ON -DANTHEM_BUILD_TESTS=ON
ninja
ninja run-tests
strip bin/anthem
version=$(git describe --tags)
version=${version#v}
output_directory=$tmp_directory/anthem-$version
popd

# prepare archive
mkdir -p $output_directory
rm -rf $output_directory/*
install -m 755 -d $output_directory/examples

# copy files
install -m 755 $build_directory/bin/anthem $output_directory/
install -m 755 /home/patrick/Documents/ASP/vampire/vampire_rel_master_4013 $output_directory/vampire

for f in $(git ls-files | grep 'examples/')
do
	echo $f
	install -Dm 644 $f $output_directory/$f
done

install -m 644 CHANGELOG.md $output_directory/
install -m 644 LICENSE.md $output_directory/
install -m 644 README.md $output_directory/

archive_filename=anthem-$version-linux-x86_64.tar.gz

# create archive
pushd $tmp_directory
tar -zcf $archive_filename anthem-$version
popd

mv $tmp_directory/$archive_filename .

# sign archive and show checksums
gpg --yes --output $archive_filename.sig --detach-sig $archive_filename

echo '   md5sum:' $(md5sum $archive_filename)
echo 'sha256sum:' $(sha256sum $archive_filename)
echo 'sha512sum:' $(sha512sum $archive_filename)
