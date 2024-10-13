all: help

.PHONY: ipa apk sign plugins

GODOT=/Users/john/code/godot/bin/godot.macos.editor.x86_64

BUILDS=../builds
BUILDS_RELEASE=../builds_release

help:
	@echo To build an ios
	@echo "  make ipa"
	@echo "  make ipa_release           -- Generates a release for Apple"
	@echo "  make install_ios           -- Installs current debug build"
	@echo "  make install_ios_release"
	@echo "  make sign                  -- To convert the game.ipa in windows installable"
	@echo "  make upload_ios_build      -- Will upload current ios release to apple"
	@echo "  make plugin_ios            -- Recompile C++ plugins for ios debug"
	@echo "  make plugin_ios_release    --                            ...release"
	@echo "  "
	@echo To build an android
	@echo "  make android_plugin        -- Build Android Plugin (kotlin code)"
	@echo "  make apk                   -- Build Android binary (no plugins recompiled)"
	@echo "  make apk_release           -- Recompiles in release mode"
	@echo "  make install_apk           -- Installs debug build"
	@echo "  make install_apk_release   -- Installs release build"
	@echo "  make android_build         -- Compile and start in debug mode"
	@echo "  "

# scons platform=ios target=template_release
# vtool -show game.app/Frameworks/libexpedition.ios.release.universal.framework/libexpedition.ios.release.universal
# ios-deploy -b ~/Archives/2024-07-04/game\ 4-7-24,\ 01.32.xcarchive/Products/Applications/game.app
# To confirm the debug symbols are there
# nm -a .os
# nm -a bin/libfuji.ios.template_debug.universal.dylib   

# apk/aab in debug
apk:
	@make check_android
	${GODOT} --headless --export-debug Android $(BUILDS)/game.aab

android_build:
	make android_plugin && make apk && make install_apk && make log_apk

android_plugin:
	echo "Generating Android Plugin"
	cd ../android-plugin && ./gradlew assemble

ipa:
	mkdir -p $(BUILDS)
	${GODOT} --headless --export-debug iOS $(BUILDS)/game.ipa

ipa_release:
	@make plugin_ios_release
	rm -rf $(BUILDS_RELEASE)
	mkdir -p $(BUILDS_RELEASE)
	@node ../tools/inc_build_number ios
	@echo ✅ Increased iOS build number
	${GODOT} --headless --export-release iOS $(BUILDS_RELEASE)/game.ipa 

check_android:
	@echo ✅ Checking godot Plugin configuration for Android
	@if ! grep -q "GodotAndroidPluginExpedition/plugin.cfg" project.godot; then \
		echo "Error: 'GodotAndroidPluginExpedition/plugin.cfg' not found in project.godot"; \
		exit 1; \
	fi

apk_release:
	rm -rf $(BUILDS_RELEASE)
	mkdir -p $(BUILDS_RELEASE)
	make android_plugin
	@echo ⏩ Generating Expedition Plugin
	cd .. && scons platform=android target=template_release
	cd .. && scons platform=android target=template_release arch=armv7
	@echo ✅ Generated Expedition Plugin
	@make check_android
	make inc_apk_build_number
	@echo ⏩ Generating App
	${GODOT} --headless --export-release Android $(BUILDS_RELEASE)/game.aab
	@echo ✅ Generated Android App
	make install_apk_release

check_defined = \
    $(if $(value $1),,$(error Error: Env variable $1 is not set or is empty.))

upload_ios_build:
	@echo ⏩ Uploading iOS build to Apple
	$(call check_defined,IOS_UPLOAD_USER)
	$(call check_defined,IOS_UPLOAD_PASS)
	@xcrun altool --upload-app -f $(BUILDS_RELEASE)/game.ipa -t ios -u ${IOS_UPLOAD_USER} -p "${IOS_UPLOAD_PASS}"
	@echo ✅ Upload complete

inc_apk_build_number:
	@node ../tools/inc_build_number android
	@echo ✅ Increased Android build number

plugin_ios:
	cd .. && scons platform=ios ios_min_version=18.0 debug_symbols=yes
#	cd .. && scons platform=android

plugin_ios_release:
	@echo ⏩ Compiling plugins for iOS
	cd .. && scons platform=ios target=template_release ios_min_version=18.0
	mv bin/libexpedition.ios.template_release.universal.dylib bin/libexpedition.ios.release.universal.dylib
	@echo ✅ Compiled plugins for iOS

install_apk :
	echo "Installing App"
	#${ANDROID_HOME}/platform-tools/adb install builds_release/game.aab
	rm -rf builds_apk/game.apks
	bundletool build-apks --bundle=$(BUILDS)/game.aab --output=builds_apk/game.apks --local-testing --ks=prod.keystore --ks-key-alias=androiddebugkey --ks-pass=pass:aeXvXw3L9niXRgPxONp3
	bundletool install-apks --apks=builds_apk/game.apks
	rm -rf builds_apk

install_apk_release :
	echo "Installing App"
	#${ANDROID_HOME}/platform-tools/adb install builds_release/game.aab
	rm -rf builds_release_apk/game.apks
	bundletool build-apks --bundle=$(BUILDS_RELEASE)/game.aab --output=builds_release_apk/game.apks --local-testing --ks=prod.keystore --ks-key-alias=androiddebugkey --ks-pass=pass:aeXvXw3L9niXRgPxONp3
	bundletool install-apks --apks=builds_release_apk/game.apks
	rm -rf builds_release_apk

install_ios : 
	rm -rf $(BUILDS)/ipa
	mkdir -p $(BUILDS)/ipa
	unzip $(BUILDS)/game.ipa -d $(BUILDS)/ipa
	ios-deploy -b $(BUILDS)/ipa/Payload/game.app

install_ios_release : 
	rm -rf $(BUILDS_RELEASE)ipa
	mkdir -p $(BUILDS_RELEASE)/ipa
	unzip $(BUILDS_RELEASE)/game.ipa -d $(BUILDS_RELEASE)/ipa
	ios-deploy -b $(BUILDS_RELEASE)/ipa/Payload/game.app

log_apk:
	adb logcat | grep -i godot

android_keys:
	keytool -keyalg RSA -keysize 2048 -validity 10000 -genkey -v -keystore debug.keystore -alias androiddebugkey 
	#keytool -keyalg RSA -keysize 2048 -validity 10000 -genkey -v -keystore prod.keystore -alias androidprodkey 
	#keytool -keyalg RSA -genkeypair -alias androiddebugkey -keypass android -keystore debug.keystore -storepass android -dname "CN=Android Debug,O=Android,C=US" -validity 9999 -deststoretype pkcs12
	#keytool -v -genkey -keystore nergys.keystore -alias nergys -keyalg RSA -validity 10000

android_sha1:
	keytool -keystore prod.keystore -list -v

android_install_sdk:
	sdkmanager --sdk_root=$ANDROID_HOME "platform-tools" "build-tools;33.0.2" "platforms;android-33" "cmdline-tools;latest" "cmake;3.10.2.4988404" "ndk;23.2.8568313"

PLIST_BUDDY=/usr/libexec/PlistBuddy
XCENT_FILE=app.xcent
PROJECT_NAME=nergys
TEAM_IDENTIFIER=NUTQVSBL46
APP_BUNDLE_IDENTIFIER=com.expedition.${PROJECT_NAME}
BUNDLE_DIR=$(BUILDS)/ipa/Payload/game.app
# List signing identities: security find-identity -v -p codesigning
IDENTITY=DCF2EC1B4903712A423E3E30B50A95ED12FC00CB

# To generate an .ipa with the plist in text-mode, unzip, update, zip and sign again
sign: 
	rm -rf $(BUILDS)/ipa
	mkdir -p $(BUILDS)/ipa
	unzip $(BUILDS)/game.ipa -d $(BUILDS)/ipa
	${PLIST_BUDDY} -x -c Print ${BUNDLE_DIR}/Info.plist > ${BUNDLE_DIR}/Info.plist.tmp
	rm ${BUNDLE_DIR}/Info.plist
	mv ${BUNDLE_DIR}/Info.plist.tmp ${BUNDLE_DIR}/Info.plist
	rm -f ${XCENT_FILE}
	${PLIST_BUDDY} -c "Add :application-identifier string ${TEAM_IDENTIFIER}.${APP_BUNDLE_IDENTIFIER}" ${XCENT_FILE}
	${PLIST_BUDDY} -c "Add :com.apple.developer.team-identifier string ${TEAM_IDENTIFIER}" ${XCENT_FILE}
	${PLIST_BUDDY} -c "Add :get-task-allow bool true" ${XCENT_FILE}
	/usr/bin/codesign \
	 		--force \
	 		--timestamp=none \
	 		--sign ${IDENTITY} \
	 		--entitlements ${XCENT_FILE} \
	 		--generate-entitlement-der \
	 		${BUNDLE_DIR}
	@rm -f ${XCENT_FILE}
	cd $(BUILDS)/ipa && zip -r ../../game.ipa Payload

