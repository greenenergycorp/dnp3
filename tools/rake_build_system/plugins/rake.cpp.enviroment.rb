
# when buidling for ARM we have to modify PATH to point first to the cross compiler
if ENV['arm']
  ENV['PATH'] = case $hw_os
    when 'pc_cygwin'
      '/opt/crosstool/gcc-3.3.4-glibc-2.3.2/arm-unknown-linux-gnu/arm-unknown-linux-gnu/bin/:/opt/crosstool/gcc-3.3.4-glibc-2.3.2/arm-unknown-linux-gnu/bin/:' + ENV['PATH']
    when 'Linux_i686'
      '/usr/local/opt/crosstool/arm-linux/gcc-3.3.4-glibc-2.3.2/bin:/usr/local/opt/crosstool/arm-linux/gcc-3.3.4-glibc-2.3.2/arm-linux/bin:' + ENV['PATH']
    else
      puts "Don't know what cross-compiler to use for #{$hw_os}"
      Process.exit(1)
    end
  $hw_os = 'pc_linux_arm'
end

# defaults for compiler, arhiver, and indexer
$CC = ENV['ccache'] ? 'ccache g++' : 'g++'

$AR = 'ar cruv'
$RANLIB = 'ranlib'
$LD = 'ld'

COMPILER_VERSION = case $hw_os
 when 'pc_cygwin'
  '-gcc34'
 when 'Linux_i686'
  '-gcc41'
 when 'pc_linux_arm'
  '-gcc33'
end

preprocessor = case $hw_os
 when 'pc_cygwin'
  ['APL_CYGWIN']
 when 'pc_linux_arm'
  ['ARM']
 else []
end

if ENV['PREPROCESSOR']
  preprocessor << ENV['PREPROCESSOR']
end

$CC_PREPROCESSOR = preprocessor.collect { |d| "-D #{d}"}.join(' ')

$WINSOCK_LIBS = case $hw_os
  when 'pc_cygwin'
    ['/lib/w32api/libwsock32.a','/lib/w32api/libws2_32.a']
  else
    []
end

#By default, the build is set to debug
$WARN_FLAGS   = ['-Wall']
$RELEASE_TYPE = ENV['release'] ? 'release' : (ENV['coverage'] ? 'coverage' : 'debug')
$CC_FLAGS     = case $RELEASE_TYPE
  when 'release'
    $hw_os == 'pc_linux_arm' ? ['-O3'] : ['-O3 -fPIC']
  when 'debug'
    $hw_os == 'pc_linux_arm' ? []      : ['-g -fPIC']      # no gdb on arm, so no debug needed
  when 'coverage'
    $hw_os == 'pc_linux_arm' ? []      : ['-g -fPIC -O0 -fprofile-arcs -ftest-coverage -DNDEBUG -DPSI_LOGALL']
  else
    puts "Unknown $RELEASE_TYPE = #{$RELEASE_TYPE}"
    Process.exit 1
end

$OBJ_DIR = "#{$hw_os}/#{$RELEASE_TYPE}"

$PLATFORM_LIBS = case $hw_os
  when 'pc_cygwin'
    ['-lpthread']
  when 'Linux_i686'
    ['-lpthread']
  when 'pc_linux_arm'
    ['-lpthread']
  else
    []
end

RUBY_INC = case $hw_os
  when 'pc_cygwin'
    '/lib/ruby/1.8/i386-cygwin'
  when 'Linux_i686'
    '/usr/lib/ruby/1.8/i486-linux/'
end

$PLATFORM_LIBS << '-lgcov' if $RELEASE_TYPE == 'coverage'
