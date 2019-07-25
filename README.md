# tapcat

A minimalistic ucspi utility to concatenate and redirect `tap` devices.

## Usage

Create `tap0` device on both local and remote machines and turn them on
```sh
ip tuntap add tap0 mode tap user user_name group group_name
ip link set dev tap0 up
```

### Creating vpn tunnel over ssh

```sh
tapcat tap0 ssh user@server tapcat tap0
```

### Creating insecure tunnel over TCP with ncat

Server side:
```sh
ncat -c 'tapcat tap0' -klp 1234
```

Client side:
```sh
ncat -c 'tapcat tap0' 10.0.0.2 1234
```
