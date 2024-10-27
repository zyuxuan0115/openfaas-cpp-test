import yaml
import sys
import os

def main():
  if sys.argv[1] == 'openfaas':
    openfaas()
  elif sys.argv[1] == 'nginx':
    nginx()

def openfaas():
  if os.path.exists("openfaas2.yml"):
    os.remove("openfaas2.yml")
  docs = yaml.safe_load_all(sys.stdin)
  with open('openfaas2.yml', 'a') as outfile:
    for doc in docs:
      if doc['kind'] == 'Namespace' and doc['metadata']['name'] == 'openfaas':
        doc['metadata']['name'] = 'openfaas2'
      elif doc['kind'] == 'Namespace' and doc['metadata']['name'] == 'openfaas-fn':
        doc['metadata']['name'] = 'openfaas2-fn'
      elif doc['kind'] == 'Deployment' and doc['metadate']['name'] == 'queue-worker':
        doc['spec']['replicas'] = 5
      doc_yml = yaml.dump(doc)
      outfile.write('---\n')
      outfile.write(doc_yml)
  outfile.close()

def nginx():
  docs = yaml.safe_load_all(sys.stdin)
  # generate the yaml file for reloading the controller
  if os.path.exists("ingress-nginx-values.yaml"):
    os.remove("ingress-nginx-values.yaml")
  docs2 = []
  with open('ingress-nginx-values.yaml', 'a') as outfile:
    for doc in docs:
      docs2.append(doc)
      if doc['kind'] == 'Service' and doc['metadata']['name'] == 'ingress-nginx-controller':
        for item in doc['spec']['ports']:
          if item['appProtocol'] == 'http':
            item['nodePort'] = 30080
          elif item['appProtocol'] == 'https':
            item['nodePort'] = 30443
        doc_yaml = yaml.dump(doc)
        outfile.write('---\n')
        outfile.write(doc_yaml)
      elif doc['kind'] == 'Deployment' and doc['metadata']['name'] == 'ingress-nginx-controller':
        doc_yaml = yaml.dump(doc)
        outfile.write('---\n')
        outfile.write(doc_yaml)
  outfile.close()
  # generate the yaml file for nginx-ingress-controller2
  if os.path.exists("ingress-nginx2.yaml"): 
    os.remove("ingress-nginx2.yaml")
  docs3 = []
  with open('ingress-nginx2.yaml', 'a') as outfile2: 
    for doc in docs2:
      if doc['kind'] == 'Namespace':
        doc['metadata']['name'] = 'ingress-nginx2'
      if doc['kind'] == 'IngressClass': 
         doc['metadata']['name'] = 'nginx2'
         doc['spec']['controller'] = 'k8s.io/ingress-nginx2'
      if doc['kind'] == 'Deployment':
        if 'spec' in doc and 'template' in doc['spec'] and 'spec' in doc['spec']['template']:
          if 'containers' in doc['spec']['template']['spec']:
            for item in doc['spec']['template']['spec']['containers']:
              new_args = []
              for arg in item['args']:
                print(arg)
                if arg == '--controller-class=k8s.io/ingress-nginx':
                  new_args.append('--controller-class=k8s.io/ingress-nginx2')
                elif arg == '--ingress-class=nginx':
                  new_args.append('--ingress-class=nginx2')
                else:
                  new_args.append(arg)
              item['args'] = new_args
      if 'metadata' in doc and 'namespace' in doc['metadata']:
        doc['metadata']['namespace'] = 'ingress-nginx2'
      if 'subjects' in doc:
        for item in doc['subjects']:
          if 'namespace' in item:
            item['namespace'] = 'ingress-nginx2'
      if 'webhooks' in doc:
        for item in doc['webhooks']:
          if 'clientConfig' in item and 'service' in item['clientConfig'] and 'namespace' in item['clientConfig']['service']:
            item['clientConfig']['service']['namespace'] = 'ingress-nginx2'
      docs3.append(doc)
      doc_yaml = yaml.dump(doc)
      outfile2.write('---\n')
      outfile2.write(doc_yaml)
  outfile2.close()
  # generate the yaml file for reloading the controller2
  if os.path.exists("ingress-nginx2-values.yaml"): 
    os.remove("ingress-nginx2-values.yaml")
  with open('ingress-nginx2-values.yaml', 'a') as outfile3: 
    for doc in docs3:
      if doc['kind'] == 'Service' and doc['metadata']['name'] == 'ingress-nginx-controller':
        for item in doc['spec']['ports']:
          if item['appProtocol'] == 'http':
            item['nodePort'] = 30081
          elif item['appProtocol'] == 'https':
            item['nodePort'] = 30442
        doc_yaml = yaml.dump(doc)
        outfile3.write('---\n')
        outfile3.write(doc_yaml)
      elif doc['kind'] == 'Deployment' and 'metadata' in doc and 'namespace' in doc['metadata'] and doc['metadata']['name'] == 'ingress-nginx-controller':
        doc_yaml = yaml.dump(doc)
        outfile3.write('---\n')
        outfile3.write(doc_yaml)
  outfile3.close()


if __name__=="__main__": 
  main() 
