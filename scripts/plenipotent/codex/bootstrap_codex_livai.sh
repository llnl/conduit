#!/bin/bash
#
# Note: This is for use *only* in isolated environments, such as docker development envs.
#       Not suitable for use directly on LLNL systems (LC or otherwise).
#
#       This should only be used to develop released open source code.
#
test -e `command -v npm` || { echo "npm is required to run codex"; exit 1; }
test -e `command -v codex` || { echo "codex is missing (npm install -g @openai/codex)"; exit 1; }
here=$(dirname "$(readlink -f "$0")")
user_config=$HOME/.codex/config.toml
livai_key_file=$HOME/.livai-api-key.txt
reference_config=$here/codex_config_livai.toml
test -e $livai_key_file || { echo "please add your livai api key to $livai_key_file"; exit 1; }
test -e $user_config || { echo "please \`cp $reference_config $user_config'"; exit 1; }
export LIVAI_API_KEY=$(cat $livai_key_file)
codex "$@"