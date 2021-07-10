pipeline {
    agent {
        docker { image 'nxs/build' }
    }
    environment {
        NXS_COMPILER='gcc'
        NXS_GENERATOR='ninja'
        NXS_BUILD_TYPE='release'
        NXS_WORKLOG="${WORKSPACE}/../data/worklog"
        BUILD_DIR="build/${NXS_BUILD_TYPE}-${NXS_COMPILER}"
        DEPLOY_PATH="${WORKSPACE}/../bin"
    }
    stages {
        stage('Init') {
            steps {
                sh 'rm -rfv "${DEPLOY_PATH}" && mkdir -p "${DEPLOY_PATH}"'
                sh 'rm -rfv build/artifacts && mkdir -p build/artifacts'
                sh 'rm -rfv "${NXS_WORKLOG}" && mkdir -p "${NXS_WORKLOG}"'
            }
        }
        stage('PreChecks') {
            steps {
                sh 'ci/pre-checks.sh'
            }
        }
        stage('Build') {
            steps {
                sh 'tools/build.sh --path "${WORKSPACE}" -j $(nproc)'
            }
        }
        stage('Lint') {
            steps {
                warnError(message: 'Lint failed') {
                    sh 'ci/lint.sh'
                }
            }
        }
        stage('Unit Test') {
            steps {
                sh '${BUILD_DIR}/bin/test_nexus'
            }
        }
        stage('Deploy') {
            steps {
                sh 'ci/deploy.sh'
            }
        }
        stage('Function Test') {
            steps {
                sh 'ci/test.sh'
            }
        }
    }
    post {
        always {
            archiveArtifacts artifacts: 'build/artifacts/**/*', fingerprint: true
        }
    }
}
