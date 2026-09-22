# Domain saflığı beyan değil, kontrol. TC-009 I/O'yu, TC-012 QObject'i yasaklar;
# ikisi de ancak burada bozulduğu anda görünür olursa geçerlidir.
file(GLOB_RECURSE sources "${DIR}/src/*.cpp" "${DIR}/src/*.h" "${DIR}/include/*.h")
set(forbidden "QFile|QProcess|QDir|QTextStream|QDateTime::current|Q_OBJECT|Q_GADGET")
set(violations "")
foreach(file IN LISTS sources)
    file(READ "${file}" text)
    # Yorum satırları sayılmaz: yasağın kendisini anlatan yorum ihlal değildir.
    string(REGEX REPLACE "//[^\n]*" "" text "${text}")
    if(text MATCHES "${forbidden}")
        list(APPEND violations "${file}")
    endif()
endforeach()
if(violations)
    message(FATAL_ERROR "domain saflığı bozuldu (${forbidden}): ${violations}")
endif()
message(STATUS "domain saf: ${forbidden} yok")
