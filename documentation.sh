#!/bin/bash
set -e # Exit with nonzero exit code if anything fails

SOURCE_BRANCH="master"
TARGET_BRANCH="gh-pages"
OUTPUT_PATH="$TRAVIS_BUILD_DIR/output"
COMMIT_AUTHOR_EMAIL="${COMMIT_AUTHOR_EMAIL:-build@tarantool.org}"

function doCompile {
    mkdir benv
    virtualenv benv
    source benv/bin/activate
    pip install sphinx sphinx_rtd_theme
    cmake .
    make sphinx-html
    deactivate
    rm -rf benv
}

# Pull requests and commits to other branches shouldn't try to deploy, just
# build to verify
if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "$SOURCE_BRANCH" ]; then
    echo "documentation.sh: Skipping deploy; just doing a build."
    doCompile
    exit 0
fi

# Save some useful information
REPO=`git config remote.origin.url`
SSH_REPO=${REPO/https:\/\/github.com\//git@github.com:}
SHA=`git rev-parse --verify HEAD`

# Clone the existing gh-pages for this repo into $OUTPUT_PATH
# Create a new empty branch if gh-pages doesn't exist yet (should only happen
# on first deply)
git clone $REPO $OUTPUT_PATH
cd $OUTPUT_PATH
git checkout $TARGET_BRANCH || git checkout --orphan $TARGET_BRANCH
cd $TRAVIS_BUILD_DIR

# Clean out existing contents
rm -rf $OUTPUT_PATH/* || exit 0

# Run our compile script
doCompile

# Now let's go have some fun with the cloned repo
cd $OUTPUT_PATH
git config user.name "Travis CI"
git config user.email "$COMMIT_AUTHOR_EMAIL"

# If there are no changes to the compiled out (e.g. this is a README update)
# then just bail.
if [ -z "`git diff --stat --exit-code`" ]; then
    echo "documentation.sh: No changes to the output on this push; exiting."
    exit 0
fi

# Commit the "changes", i.e. the new version.
# The delta will show diffs between new and old versions.
git add --all .
git status
git commit -m "Deploy to GitHub Pages: ${SHA}"

# Get the deploy key by using Travis's stored variables to decrypt deploy_key.enc
ENCRYPTED_KEY_VAR="encrypted_${ENCRYPTION_LABEL}_key"
ENCRYPTED_IV_VAR="encrypted_${ENCRYPTION_LABEL}_iv"
ENCRYPTED_KEY=${!ENCRYPTED_KEY_VAR}
ENCRYPTED_IV=${!ENCRYPTED_IV_VAR}
ENCRYPTED_KEY_PATH="${TRAVIS_BUILD_DIR}/deploy_key.enc"
DECRYPTED_KEY_PATH="${TRAVIS_BUILD_DIR}/deploy_key"
openssl aes-256-cbc -K $ENCRYPTED_KEY -iv $ENCRYPTED_IV -in "$ENCRYPTED_KEY_PATH" -out "$DECRYPTED_KEY_PATH" -d
chmod 600 "$TRAVIS_BUILD_DIR/deploy_key"
eval `ssh-agent -s`
ssh-add "$TRAVIS_BUILD_DIR/deploy_key"

# Now that we're all set up, we can push.
git push $SSH_REPO $TARGET_BRANCH
