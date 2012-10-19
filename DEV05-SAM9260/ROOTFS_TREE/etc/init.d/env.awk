{
	split ($0, x, "=");

	# ====================
	# == eth0 interface ==
	# ====================

	if (x[1] ~ /eth0/)
	{
		# == MAC address ==

		if (x[1] ~ /eth0.mac/)
		{
			if ("eth0.mac.addr" == x[1])
				eth0_mac_addr = x[2];
		}

		# == DHCP option ==

		else if (x[1] ~ /eth0.dhcp/)
		{
			if ("eth0.dhcp" == x[1])
				eth0_dhcp = ("on" == x[2] || "off" == x[2]) ? x[2] : "on";
			else if ("eth0.dhcp.start" == x[1])
				eth0_dhcp_start = x[2];
			else if ("eth0.dhcp.end" == x[1])
				eth0_dhcp_end = x[2];
			else if ("eth0.dhcp.netmask" == x[1])
				eth0_dhcp_netmask = x[2];
			else if ("eth0.dhcp.gw" == x[1])
				eth0_dhcp_gw = x[2];
			else if ("eth0.dhcp.dns0" == x[1])
				eth0_dhcp_dns0 = x[2];
			else if ("eth0.dhcp.dns1" == x[1])
				eth0_dhcp_dns1 = x[2];
			else if ("eth0.dhcp.lease" == x[1])
				eth0_dhcp_lease = x[2];
		}

		# == ip address ==

		else if (x[1] ~ /eth0.ip/)
		{
			if ("eth0.ip0.addr" == x[1])
				eth0_ip0_addr = x[2];
			else if ("eth0.ip0.netmask" == x[1])
				eth0_ip0_netmask = x[2];

			else if ("eth0.ip1.addr" == x[1])
				eth0_ip1_addr = x[2];
			else if ("eth0.ip1.netmask" == x[1])
				eth0_ip1_netmask = x[2];

			else if ("eth0.ip2.addr" == x[1])
				eth0_ip2_addr = x[2];
			else if ("eth0.ip2.netmask" == x[1])
				eth0_ip2_netmask = x[2];

			else if ("eth0.ip3.addr" == x[1])
				eth0_ip3_addr = x[2];
			else if ("eth0.ip3.netmask" == x[1])
				eth0_ip3_netmask = x[2];
		}

		# == gateway ==

		else if (x[1] ~ /eth0.gw/)
		{
			if (x[1] ~ /eth0.gw0/)
			{
				if ("eth0.gw0.addr" == x[1])
					eth0_gw0_addr = x[2];
				else if ("eth0.gw0.metric" == x[1])
					eth0_gw0_metric = x[2];

				else if (x[1] ~ /eth0.gw0.inspect/)
				{
					if ("eth0.gw0.inspect.type" == x[1])
						eth0_gw0_inspect_type = ("tcp" == x[2] || "icmp" == x[2] || "all" == x[2]) ? x[2] : "";
					else if ("eth0.gw0.inspect.threshold" == x[1])
						eth0_gw0_inspect_threshold = x[2];
					else if ("eth0.gw0.inspect.addr" == x[1])
						eth0_gw0_inspect_addr = x[2];
					else if ("eth0.gw0.inspect.port" == x[1])
						eth0_gw0_inspect_port = x[2];
					else if ("eth0.gw0.inspect.interval" == x[1])
						eth0_gw0_inspect_interval = x[2];
					else if ("eth0.gw0.inspect.timeout" == x[1])
						eth0_gw0_inspect_timeout = x[2];
				}
			}
			else if (x[1] ~ /eth0.gw1/)
			{
				if ("eth0_gw1.addr" == x[1])
					eth0_gw1_addr = x[2];
				else if ("eth0.gw1.metric" == x[1])
					eth0_gw1_metric = x[2];

				else if (x[1] ~ /eth0.gw1.inspect/)
				{
					if ("eth0.gw1.inspect.type" == x[1])
						eth0_gw1_inspect_type = ("tcp" == x[2] || "icmp" == x[2] || "all" == x[2]) ? x[2] : "";
					else if ("eth0.gw1.inspect.threshold" == x[1])
						eth0_gw1_inspect_threshold = x[2];
					else if ("eth0.gw1.inspect.addr" == x[1])
						eth0_gw1_inspect_addr = x[2];
					else if ("eth0.gw1.inspect.port" == x[1])
						eth0_gw1_inspect_port = x[2];
					else if ("eth0.gw1.inspect.interval" == x[1])
						eth0_gw1_inspect_interval = x[2];
					else if ("eth0.gw1.inspect.timeout" == x[1])
						eth0_gw1_inspect_timeout = x[2];
				}
			}
		}

		# == dns ==

		else if ("eth0.dns0.addr" == x[1])
			eth0_dns0_addr = x[2];
		else if ("eth0.dns1.addr" == x[1])
			eth0_dns1_addr = x[2];
	}
}
END
{
	if ("all" == ifname || "eth0" == ifname)
	{
		# ===============================
		# == deactivate eth0 interface ==
		# ===============================

		system ("ifconfig eth0 down");

		# ======================
		# == eth0 mac address ==
		# ======================

		if ("" != eth0_mac_addr)
			system ("ifconfig eth0 hw ether " eth0_mac_addr);

		# =============================
		# == activate eth0 interface ==
		# =============================

		if ("" != eth0_ip0_addr && "" != eth0_ip0_netmask)
		{
			system ("ifconfig eth0 up");
		}
	}

	if ("eth0" == ifname || "all" == ifname)
	{
		# =========================================
		# == check dhcp server existence on eth0 ==
		# =========================================

		if ("on" == eth0_dhcp && "" != eth0_ip0_addr && "" != eth0_ip0_netmask)
		{
			system ("ifconfig eth0 " eth0_ip0_addr " netmask " eth0_ip0_netmask);

			if (0 == system ("udhcpc -i eth0 -R -n -q -t 2 -T 2"))
				eth0_dhcp = "off";
		}
	}

	# ==========================================
	# == delete all gwd for set all interface ==
	# ==========================================

	if ("all" == ifname)
	{
		system ("killall -q gwd");
	}
	else if ("eth0" == ifname)
	{
		system ("waitfor -t 1000 -k -m '-i eth0' gwd");
	}

	if ("eth0" == ifname || "all" == ifname)
	{
		# =================================
		# == eth0 ip address and gateway ==
		# =================================

		if ("" != eth0_ip0_addr && "" != eth0_ip0_netmask)
			system ("ifconfig eth0 " eth0_ip0_addr " netmask " eth0_ip0_netmask);
		if ("" != eth0_ip1_addr && "" != eth0_ip1_netmask)
			system ("ifconfig eth0:1 " eth0_ip1_addr " netmask " eth0_ip1_netmask);
		if ("" != eth0_ip2_addr && "" != eth0_ip2_netmask)
			system ("ifconfig eth0:2 " eth0_ip2_addr " netmask " eth0_ip2_netmask);
		if ("" != eth0_ip3_addr && "" != eth0_ip3_netmask)
			system ("ifconfig eth0:3 " eth0_ip3_addr " netmask " eth0_ip3_netmask);
		if ("" != eth0_gw0_addr && "" != eth0_gw0_metric)
			system ("route add default gw " eth0_gw0_addr " metric " eth0_gw0_metric " dev eth0");
		if ("" != eth0_gw1_addr && "" != eth0_gw1_metric)
			system ("route add default gw " eth0_gw1_addr " metric " eth0_gw1_metric " dev eth0");

		system ("ip rule del fwmark 10");
		system ("ip route flush table 10");

		system ("ip rule del fwmark 11");
		system ("ip route flush table 11");

		system ("iptables -t mangle -N INSPECT_ETH0");
		system ("iptables -t mangle -F INSPECT_ETH0");
		system ("iptables -t mangle -D OUTPUT -j INSPECT_ETH0");
		system ("iptables -t mangle -I OUTPUT -j INSPECT_ETH0");

		system ("iptables -t nat -N INSPECT_ETH0");
		system ("iptables -t nat -F INSPECT_ETH0");
		system ("iptables -t nat -D POSTROUTING -j INSPECT_ETH0");
		system ("iptables -t nat -I POSTROUTING -j INSPECT_ETH0");

		if ("" != eth0_gw0_addr && "" != eth0_gw0_metric && "" != eth0_gw0_inspect_type && "" != eth0_gw0_inspect_threshold && "" != eth0_gw0_inspect_addr && (("" != eth0_gw0_inspect_port && "tcp" == eth0_gw0_inspect_type) || "icmp" == eth0_gw0_inspect_type))
		{
			if (2000 > eth0_gw0_inspect_interval + 0)
			{
				eth0_gw0_inspect_interval = 2000;
			}

			if (5000 > eth0_gw0_inspect_timeout + 0)
			{
				eth0_gw0_inspect_timeout = 5000;
			}

			system ("ip route add default via " eth0_gw0_addr " dev eth0 table 10");
			system ("ip rule add fwmark 10 table 10");

			if ("tcp" == eth0_gw0_inspect_type)
			{
				system ("iptables -t mangle -A INSPECT_ETH0 -p tcp -d " eth0_gw0_inspect_addr "/32 --dport " eth0_gw0_inspect_port " -j MARK --set-mark 10");
				system ("iptables -t nat -A INSPECT_ETH0 -o eth0 -p tcp -d " eth0_gw0_inspect_addr "/32 --dport " eth0_gw0_inspect_port " -j SNAT --to-source " eth0_ip0_addr);
				system ("gwd -i eth0 -thd " eth0_gw0_inspect_threshold " -t " eth0_gw0_inspect_timeout " -gap " eth0_gw0_inspect_interval " -gw " eth0_gw0_addr " -metric " eth0_gw0_metric " -p " eth0_gw0_inspect_type " " eth0_gw0_inspect_addr " " eth0_gw0_inspect_port " &");
			}
			else if ("icmp" == eth0_gw0_inspect_type)
			{
				system ("iptables -t mangle -A INSPECT_ETH0 -p icmp -d " eth0_gw0_inspect_addr "/32 -j MARK --set-mark 10");
				system ("iptables -t nat -A INSPECT_ETH0 -o eth0 -p icmp -d " eth0_gw0_inspect_addr "/32 -j SNAT --to-source " eth0_ip0_addr);
				system ("gwd -i eth0 -thd " eth0_gw0_inspect_threshold " -t " eth0_gw0_inspect_timeout " -gap " eth0_gw0_inspect_interval " -gw " eth0_gw0_addr " -metric " eth0_gw0_metric " -p " eth0_gw0_inspect_type " " eth0_gw0_inspect_addr " " eth0_gw0_inspect_port " &");
			}
		}

		if ("" != eth0_gw1_addr && "" != eth0_gw1_metric && "" != eth0_gw1_inspect_type && "" != eth0_gw1_inspect_threshold && "" != eth0_gw1_inspect_addr && (("" != eth0_gw1_inspect_port && "tcp" == eth0_gw1_inspect_type) || "icmp" == eth0_gw1_inspect_type))
		{
			if (2000 > eth0_gw1_inspect_interval + 0)
			{
				eth0_gw1_inspect_interval = 2000;
			}

			if (5000 > eth0_gw1_inspect_timeout + 0)
			{
				eth0_gw1_inspect_timeout = 5000;
			}

			system ("ip route add default via " eth0_gw1_addr " dev eth0 table 11");
			system ("ip rule add fwmark 11 table 11");

			if ("tcp" == eth0_gw1_inspect_type)
			{
				system ("iptables -t mangle -A INSPECT_ETH0 -p tcp -d " eth0_gw1_inspect_addr "/32 --dport " eth0_gw1_inspect_port " -j MARK --set-mark 11");
				system ("iptables -t nat -A INSPECT_ETH0 -o eth0 -p tcp -d " eth0_gw1_inspect_addr "/32 --dport " eth0_gw1_inspect_port " -j SNAT --to-source " eth0_ip1_addr);
				system ("gwd -i eth0 -thd " eth0_gw1_inspect_threshold " -t " eth0_gw1_inspect_timeout " -gap " eth0_gw1_inspect_interval " -gw " eth0_gw1_addr " -metric " eth0_gw1_metric " -p " eth0_gw1_inspect_type " " eth0_gw1_inspect_addr " " eth0_gw1_inspect_port " &");
			}
			else if ("icmp" == eth0_gw1_inspect_type)
			{
				system ("iptables -t mangle -A INSPECT_ETH0 -p icmp -d " eth0_gw1_inspect_addr "/32 -j MARK --set-mark 11");
				system ("iptables -t nat -A INSPECT_ETH0 -o eth0 -p icmp -d " eth0_gw1_inspect_addr "/32 -j SNAT --to-source " eth0_ip1_addr);
				system ("gwd -i eth0 -thd " eth0_gw1_inspect_threshold " -t " eth0_gw1_inspect_timeout " -gap " eth0_gw1_inspect_interval " -gw " eth0_gw1_addr " -metric " eth0_gw1_metric " -p " eth0_gw1_inspect_type " " eth0_gw1_inspect_addr " " eth0_gw1_inspect_port " &");
			}
		}

		system ("iptables -t mangle -A INSPECT_ETH0 -j RETURN");
		system ("iptables -t nat -A INSPECT_ETH0 -j RETURN");
	}

	# =========
	# == dns ==
	# =========

	dns_str = "";

	if ("" != eth0_dns0_addr)
		dns_str = "nameserver " eth0_dns0_addr "\n";
	if ("" != eth0_dns1_addr)
		dns_str = dns_str "nameserver " eth0_dns1_addr "\n";

	if ("" != dns_str)
		system ("echo \"" dns_str "\" > /etc/resolv.conf");

	# =================
	# == dhcp server ==
	# =================

	if ("all" == ifname)
	{
		system ("killall -q udhcpd");
	}
	else if ("eth0" == ifname)
	{
		system ("waitfor -t 1000 -k -m '/etc/udhcp_eth0.cfg' udhcpd");
	}

	if ("eth0" == ifname || "all" == ifname)
	{
		if ("on" == eth0_dhcp && "" != eth0_dhcp_start && "" != eth0_dhcp_end && "" != eth0_dhcp_netmask)
		{
			# ==============================
			# == produce dhcp config file ==
			# ==============================

			dhcp_cfg = "interface eth0\n";
			dhcp_cfg = dhcp_cfg "start " eth0_dhcp_start "\n";
			dhcp_cfg = dhcp_cfg "end " eth0_dhcp_end "\n";
			dhcp_cfg = dhcp_cfg "option subnet " eth0_dhcp_netmask "\n";
			if ("" != eth0_dhcp_gw)
				dhcp_cfg = dhcp_cfg "opt router " eth0_dhcp_gw "\n";
			if ("" != eth0_dhcp_dns0)
				dhcp_cfg = dhcp_cfg "option dns " eth0_dhcp_dns0 "\n";
			if ("" != eth0_dhcp_dns1)
				dhcp_cfg = dhcp_cfg "option dns " eth0_dhcp_dns1 "\n";
			if ("" != eth0_dhcp_lease)
				dhcp_cfg = dhcp_cfg "option lease " eth0_dhcp_lease "\n";

			system ("echo \"" dhcp_cfg "\" > " dhcp_eth0_cfg);
			system ("udhcpd " dhcp_eth0_cfg);
		}
	}
}
