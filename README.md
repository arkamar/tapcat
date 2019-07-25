# tapcat

A minimalistic ucspi utility to concatenate and redirect `tap` devices.

## Usage

Create `tap0` device on both local and remote machines and turn them on
```sh
ip tuntap add tap0 mode tap user user_name group group_name
ip link set dev tap0 up
```

Create vpn tunnel over ssh
```sh
tapcat tap0 ssh server tapcat tap0 ssh
```
