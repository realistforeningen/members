Installation:

1. `sudo apt-get update && sudo apt-get install libncurses-dev libncursesw5-dev libsqlite3-dev libssh-dev libicu-dev`
2. `git clone https://github.com/realistforeningen/members`
3. `cd members`
4. `python waf configure build`
5. `scp rf@login.ifi.uio.no:Kjellerstyret/medlemsliste/members.db .`
5. `build/rf-memberlist`

Depends on `libncurses5-dev`, `libncursesw5-dev`, `sqlite3-dev`, `libssh-dev` and `libicu-dev`.
