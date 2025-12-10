### initial data

Organization basic data
```sh
cleos push action dsguideguide saveplugin '{"pcode":"organization",
					"pname":"Organization",
					"version":"1.0.0",
					"des_cid":"","autonomous_acts":["changename","changedescid","changeperm","addplugin","approvetrx","approvenft"],
					"is_basic":true}' -p dsguideguide
```

Voting plugin data
```sh
cleos push action dsguideguide saveplugin '{"pcode":"voting",
					"pname":"Voting",
					"version":"1.0.0",
					"des_cid":"",
					"autonomous_acts":["changesr","changemaq","changevts","newvote"],
					"is_basic":false}' -p dsguideguide
```

Token plugin data
```sh
cleos push action dsguideguide saveplugin '{"pcode":"token",
					"pname":"Token",
					"version":"1.0.0",
					"des_cid":"",
					"autonomous_acts":["mint","retire"],
					"is_basic":false}' -p dsguideguide
```

Token Market plugin data
```sh
cleos push action dsguideguide saveplugin '{"pcode":"tokenmarket",
					"pname":"Token Market",
					"version":"1.0.0",
					"des_cid":"",
					"autonomous_acts":["changeconf","addexin","rmexin"],
					"is_basic":false}' -p dsguideguide
```

Mining plugin data
```sh
cleos push action dsguideguide saveplugin '{"pcode":"mining",
					"pname":"Mining",
					"version":"1.0.0",
					"des_cid":"",
					"autonomous_acts":["changeconf","addexin","rmexin"],
					"is_basic":false}' -p dsguideguide
```

NFT plugin data
```sh
cleos push action dsguideguide saveplugin '{"pcode":"nft",
					"pname":"NFT",
					"version":"1.0.0",
					"des_cid":"",
					"autonomous_acts":["create","issue","transfernft"],
					"is_basic":false}' -p dsguideguide
```


NFT Market plugin data
```sh
cleos push action dsguideguide saveplugin '{"pcode":"nftmarket",
					"pname":"NFT Market",
					"version":"1.0.0",
					"des_cid":"",
					"autonomous_acts":["sale","closesale"],
					"is_basic":false}' -p dsguideguide
```

Exchange plugin data
```sh
cleos push action dsguideguide saveplugin '{"pcode":"exchange",
					"pname":"Exchange",
					"version":"1.0.0",
					"des_cid":"",
					"autonomous_acts":["createpair","addliquidity","rmliquidity","eostotoken","tokentoeos","tokentotoken","approve","withdraw","apply"],
					"is_basic":false}' -p dsguideguide
```
### remove

```sh
cleos push action dsguideguide removeplugin '{"pcode":"plugin.name"}' -p dsguideguide

```
