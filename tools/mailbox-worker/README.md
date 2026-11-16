# TamaPoke mailbox (Cloudflare Worker)

Optional. By default the boards use [ntfy.sh](https://ntfy.sh) as the mailbox for
postcards, gifts and visits between friends who are far apart. If you prefer your
own mailbox, deploy this Worker on a free Cloudflare account. It speaks the same
API as ntfy.sh, keeps messages for 30 days and can require a key.

Everything the boards put in a mailbox is encrypted and signed with the key they
agreed on when they met, so the mailbox never sees names or contents.

```bash
npx wrangler login                        # once, opens the browser
npx wrangler kv namespace create MAIL     # put the printed id in wrangler.toml
npx wrangler secret put KEY               # any long random text (optional)
npx wrangler deploy                       # prints https://tamapoke-mailbox.<you>.workers.dev
```

Then in `config_private.h`:

```c
#define MAILBOX_URL "https://tamapoke-mailbox.<you>.workers.dev"
#define MAILBOX_KEY "the same text as KEY"
```

Boards tell their friends their own mailbox (address and key) when they meet,
so a board on ntfy.sh and a board on your Worker can still write to each other.
