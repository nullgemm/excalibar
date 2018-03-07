plugins:
	@echo "building all plugins"
	@cd alsa && make
	@cd cpu && make
	@cd click && make
	@cd example && make
	@cd mpdmonitor && make
	@cd ram && make
	@cd refresh && make
	@cd tasks && make
	@cd time && make
	@cd workspaces && make

clean:
	@echo "cleaning all plugins builds"
	@cd alsa && make clean
	@cd cpu && make clean
	@cd click && make clean
	@cd example && make clean
	@cd mpdmonitor && make clean
	@cd ram && make clean
	@cd refresh && make clean
	@cd tasks && make clean
	@cd time && make clean
	@cd workspaces && make clean

install:
	@echo "installing all plugins"
	@mkdir -p ~/.config/excalibar/plugins
	@cp ./*/bin/* ~/.config/excalibar/plugins
