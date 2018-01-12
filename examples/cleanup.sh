
find . -name project.xml.log -delete
find . -name SConstruct -delete
find . -name SConscript -delete

find . -type d -name ext -exec rm -rf "{}" \;
find . -type d -name link -exec rm -r "{}" \;
find . -type d -name src -exec rm -r "{}" \;
find . -type d -name scons -exec rm -r "{}" \;
find . -type d -name tools -exec rm -r "{}" \;