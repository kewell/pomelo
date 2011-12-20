
all: dbs syn net comport file msg thread adc din gpio power_check pulse rtc timer
.PHONY: dbs syn net comport file msg thread adc din gpio power_check pulse rtc timer
dbs: 
	cd ./dbs && $(MAKE)
syn: 
	cd ./syn && $(MAKE)	
net: 
	cd ./net && $(MAKE)		
comport: 
	cd ./comport && $(MAKE)	
file: 
	cd ./file && $(MAKE)		
msg: 
	cd ./msg && $(MAKE)		
adc: 
	cd ./adc && $(MAKE)		
din: 
	cd ./din && $(MAKE)			
gpio: 
	cd ./gpio && $(MAKE)			
power_check: 
	cd ./power_check && $(MAKE)			
pulse: 
	cd ./pulse && $(MAKE)			
rtc: 
	cd ./rtc && $(MAKE)			
timer: 
	cd ./thread && $(MAKE)			
		
clean:
	cd ./dbs && $(MAKE) clean
	cd ./syn && $(MAKE) clean
	cd ./net && $(MAKE) clean
	cd ./comport && $(MAKE) clean
	cd ./file && $(MAKE) clean
	cd ./msg && $(MAKE) clean
	cd ./thread && $(MAKE) clean
	cd ./adc && $(MAKE) clean
	cd ./din && $(MAKE) clean
	cd ./gpio && $(MAKE) clean
	cd ./power_check && $(MAKE) clean
	cd ./pulse && $(MAKE) clean
	cd ./rtc && $(MAKE) clean
	cd ./timer && $(MAKE) clean
	
