# Integration patch

`house-workshop.patch` is the exact source diff against the uploaded archive.
It has been checked with `git apply --check` against that baseline.
It does not contain Git history or rewrite your local commits.

After the isolated Linux build and manual playtest pass, review this patch in the
original repository. `git apply --check /absolute/path/to/house-workshop.patch`
checks whether the checkout still matches. Do not force a patch over later local work.
See `Docs/HOUSE_WORKSHOP.md` for outstanding checks and limitations.
