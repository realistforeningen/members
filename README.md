Installasjon:

1. `sudo apt-get update && sudo apt-get install libncurses-dev libncursesw5-dev libsqlite3-dev libssh-dev`
2. `git clone https://github.com/skjvlnd/rf`
3. `cd rf`
4. `make`
5. `scp rf@login.ifi.uio.no:Kjellerstyret/medlemsliste/members.db .`
5. `build/rf-memberlist`

Depends on `libncurses5-dev`, `libncursesw5-dev`, `sqlite3-dev`, `libssh-dev` and `libicu-dev`.
