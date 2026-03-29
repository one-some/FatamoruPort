[claire_ctrscreen screen="bottom"]

[image layer="base" storage="massageback" page="back"]
[position layer="message1" page="back" marginl="0"]
[current layer="message1" page="back"]

[locate x="26" y="25"]
[button native graphic="button_start" target="*scenario"]

[locate x="26" y="287"]
[button native graphic="button_inspect" target="*scenario"]

[locate x="26" y="470"]
[button native graphic="button_config" target="*scenario"]

[locate x="414" y="470"]
[button native graphic="button_extras" target="*scenario"]

[trans method="crossfade" time="3000" stay="nostay" children="false"]

[claire_ctrscreen screen="top"]

[image layer=base storage="オープニング" page="back"]
[trans method="crossfade" time="3000" stay="nostay" children="false"]

[wt]
[playbgm storage="女中のテーマピアノソロ"]

[image storage="オープニング" layer="base" page="fore"]
[backlay]

[s]

*scenario
[claire_ctrscreen screen="top"]
[image layer=base storage="blacksozai" page=back]
[trans method="crossfade" time="3000" stay="nostay" children="false"]

[claire_ctrscreen screen="bottom"]
[image layer=base storage="blacksozai" page=back]
[trans method="crossfade" time="3000" stay="nostay" children="false"]

[wt]
[fadeoutbgm time="5000"]

[claire_ctrscreen screen="top"]
[jump storage="scenario.ks" target="*start"]
