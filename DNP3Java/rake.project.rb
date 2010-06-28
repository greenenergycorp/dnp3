
$options = {
:allowed_platforms => ['pc_cygwin', 'Linux_i686'],
:target => 'libdnp3java.so',
:cc_release_flags => '-O1 -fPIC', #compiling the swig binding at O2 and above was causing programs to seg fault
:includes => [Boost::get_includes_dir, DEFAULT_INCLUDES],
:link_options => [],
:libs => $PLATFORM_LIBS + Boost::get_static_libs + $WINSOCK_LIBS,
:project_libs => [:dnp3, :apl],
:plugins => ['swigjava'], #swigjava specific options
:java_package => 'org.psi.dnp3', 
:java_outdir => 'jar/org/psi/dnp3',
}
if $jdk_home
  desc "Create a jar with the embedded shared library"
  task "dnp3java:package" => ["dnp3java:build"] do
    sh "cp ./DNP3Java/ExtractFromJAR.java ./DNP3Java/jar/org/psi/dnp3/ExtractFromJAR.java"
    sh "javac ./DNP3Java/jar/org/psi/dnp3/*.java"
    if ENV['windows']
      sh "cp ./DNP3Java/Release/dnp3java.dll ./DNP3Java/jar"
      java_sep = ';'
    else
      sh "rake dnp3java:copytarget[./DNP3Java/jar]"
      java_sep = ':'
    end
    sh "jar cvf ./DNP3Java/dnp3java.jar -C ./DNP3Java/jar ."
    sh "javac -cp ./DNP3Java/dnp3java.jar ./DNP3Java/TestBinding.java"
    sh "java -cp \"./DNP3Java#{java_sep}./DNP3Java/dnp3java.jar\" TestBinding"
    sh "rm -f ./DNP3Java/*.dll ./DNP3Java/*.so ./DNP3Java/*.class"
  
  end
  
  desc "Deploy the platform specific library to maven"
  task "dnp3java:deploy" => ["dnp3java:package"] do
    sh "cd DNP3Java && buildr upload"
  end
  
  desc "Install the platform specific library to the local maven repo"
  task "dnp3java:install" => ["dnp3java:package"] do
    sh "cd DNP3Java && buildr install"
  end
end
