Android/Copy     folder is copied directly to the target path overwriting existing files if necessary.
Android/Modify   folder is copied directly to the target path overwriting existing files if necessary but certain strings are replaced (for text files).


Filenames:
* Any filename that ends with ".__template__" is stripped of it.
  - So "test.txt.__template__" becomes "test.txt"

Rules for the modify folder:
- We replace the following strings in the files
  ##FSL_PACKAGE_GLES_VERSION##
  ##FSL_PACKAGE_MIN_ANDROID_SDK_VERSION##
  ##FSL_PACKAGE_TARGET_ANDROID_SDK_VERSION##
  ##PACKAGE_APP_PLATFORM##
  ##PACKAGE_DEPENDENCY_IMPORT##
  ##PACKAGE_LIBRARY_DEPENDENCIES##
  ##PACKAGE_LOCATION##
  ##PACKAGE_TARGET_NAME##
  ##PREFIXED_PROJECT_NAME##
  ##PREFIXED_PROJECT_NAME_L##


Notes:
##EXTRA_INCVPATHS##
##EXTRA_LIBVPATHS##
##EXTRA_SOURCE_OBJS##
##EXTRA_SRCVPATHS##
##FEATURE_LIST##
##FILE_PATH##
##FSL_PACKAGE_GLES_VERSION##
##FSL_PACKAGE_MIN_ANDROID_SDK_VERSION##
##FSL_PACKAGE_TARGET_ANDROID_SDK_VERSION##
##NAME##
##PACKAGE_APP_PLATFORM##
##PACKAGE_BUILD_COMMANDS##
##PACKAGE_DEFINES##
##PACKAGE_DEPENDENCY_PROJECT_PATH##
##PACKAGE_DEPENDENCY_PROJECTID##
##PACKAGE_DEPENDENCY_IMPORT##
##PACKAGE_EXTERNAL_DEBUG_LIBRARY_DEPENDENCIES##
##PACKAGE_EXTERNAL_LIBRARY_DEPENDENCIES##
##PACKAGE_EXTERNAL_LIBRARY_PATHS##
##PACKAGE_EXTERNAL_LINK_DEBUG_LIBRARY_DEPENDENCIES##
##PACKAGE_EXTERNAL_LINK_LIBRARY_DEPENDENCIES##
##PACKAGE_INCLUDE_DIRS##
##PACKAGE_LIBRARY_DEPENDENCIES##
##PACKAGE_LIBRARY_DEPENDENCIES1##
##PACKAGE_LIBRARY_DEPENDENCIES2##
##PACKAGE_LIBRARY_DEPENDENCIES3##
##PACKAGE_LOCATION##
##PACKAGE_OBJECT_PATH##
##PACKAGE_PLATFORM_PROJECT_ID##
##PACKAGE_PUBLIC_CPP_FLAGS##
##PACKAGE_PUBLIC_INCLUDE_DIRS##
##PACKAGE_SOURCE_FILES##
##PACKAGE_TARGET_INSTALL_PATH##
##PACKAGE_TARGET_NAME##
##PACKAGE_VARIANT_NAME##
##PACKAGE_VARIANT_SECTION##
##PINFO_DESC##
##PLATFORM_NAME##
##PREFIXED_PROJECT_NAME_L##
##PROJECT_VARIANT_NAME##
##SNIPPET##
##SNIPPET1##
##SNIPPET2##
##SNIPPET3##
##SNIPPET4##
##SNIPPET5##
##SNIPPET6##
##SNIPPET7##
##SNIPPET8##
##STATIC_LIBS##
##VARIANT##
##VARIANT1##
##VARIANT2##