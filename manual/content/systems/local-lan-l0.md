---
title: Local network multiplayer on iOS
summary: Finds same-Wi-Fi hosts with a specific Bonjour service and sends gameplay through OpenTS UDP peers.
category: multiplayer-networking
keys: []
---

## First accepted profile

The first iOS LAN profile is for trusted, private Wi-Fi where both devices are on the same subnet. Play stays in the foreground. The first accepted transport profile may use IPv4 only; dual-stack and IPv6 hardening follow later. The first physical check may use the same official map already present on both devices. Custom map transfer remains available in the game protocol but does not block initial LAN acceptance.

## Discovery and transport

Second Sun browses and publishes only the Bonjour service type `_secondsun._udp`. iOS declares that one value in `NSBonjourServices`, and the Local Network purpose string says local Wi-Fi is used to find and join multiplayer games. Browsing starts after the player opens **Multiplayer Game → Network**. A host publishes after creating a network game.

The service resolves a concrete private IPv4 address and UDP port. OpenTS passes that endpoint to its existing direct-peer manager, then sends the normal `NET_QUERY_GAME`, answer, join, and session messages. The gameplay path keeps the existing UDP socket, session, lobby, timing, map agreement, reliable transport, and event synchronization code. Bonjour carries discovery only. Raw UDP broadcast and Apple's multicast entitlement are not required by the iOS route.

The OpenTS default UDP port is 1234 (`WestwoodOnline_PortNumber`). Manual recovery accepts a canonical private IPv4 host address in 10/8, 172.16/12, or 192.168/16 and uses the same port and direct-peer path.

## Diagnostics and limits

A bounded 64-event diagnostic ring records numeric events for socket bind, discovery, address selection, join handshake, lobby state, map scenario, game start tick, stalls, and desync. It does not store full addresses, device IDs, player names, scenario paths, or secrets. Discovery separately reports permission not requested, access denied, no host found, discovery errors, and UDP transport errors.

The iPhoneOS compile and link proof does not establish physical permission prompts or two-device behavior. That requires a later device test. This profile does not cover WAN play, NAT traversal, CnCNet, or public servers.
